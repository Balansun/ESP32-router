#include "balansun_self_test.h"

#include "balansun_api_ready.h"
#include "balansun_globals.h"
#include "balansun_product_caps.h"
#include "balansun_self_test_safety_logic.h"
#include "balansun_self_test_safety_runtime.h"
#include "balansun_triac_sync_logic.h"
#include "storage_eeprom.h"
#include "balansun_globals.h"
#include "balansun_mains_profile.h"
#include "balansun_triac_isr.h"
#include "triac_api_shim.h"

#include <ctime>

SelfTestPersisted g_self_test;

namespace {
enum class Phase : uint8_t { Idle, ZcSample, TriacDry, SourceCheck, Done };
Phase g_phase = Phase::Idle;

static const char *phase_wire(Phase p) {
  switch (p) {
    case Phase::ZcSample:
      return "zc_sample";
    case Phase::TriacDry:
      return "triac_dry";
    case Phase::SourceCheck:
      return "source_check";
    case Phase::Done:
      return "done";
    case Phase::Idle:
    default:
      return "idle";
  }
}
unsigned long g_phase_start_ms = 0;
int g_zc_total = 0;
bool g_boot_self_test_pending = false;

bool boot_self_test_prerequisites_met() {
  if (!balansun_api_telemetry_ready()) {
    return false;
  }
  return last_metering_task_ms > 0 && (time_sync_valid || meter_reading_valid);
}
}  // namespace

void balansun_self_test_set_pending(bool pending) { g_self_test.pending = pending; }

void balansun_self_test_skip() {
  g_self_test.skipped = true;
  g_self_test.pending = false;
  g_boot_self_test_pending = false;
  g_phase = Phase::Idle;
}

void balansun_self_test_on_boot() {
  const BalansunProductCaps caps = balansun_product_caps_compile_time();
  if (!balansun_product_caps_has(caps, BalansunCap::SelfTestTriac)) {
    return;
  }
  if (g_self_test.skipped) {
    return;
  }
  g_boot_self_test_pending = true;
  g_self_test.pending = false;
  g_self_test.run_epoch = 0;
  g_phase = Phase::Idle;
}

void balansun_self_test_disarm_boot_pending() { g_boot_self_test_pending = false; }

void balansun_self_test_start_run() {
  g_boot_self_test_pending = false;
  g_self_test.skipped = false;
  g_self_test.pending = false;
  g_self_test.zc_ok = false;
  g_self_test.triac_ok = false;
  g_self_test.source_ok = false;
  g_self_test.run_epoch = 0;
  g_self_test.zc_edges_per_sec = 0;
  g_phase = Phase::ZcSample;
  g_phase_start_ms = millis();
  g_zc_total = 0;
}

bool balansun_self_test_blocks_outputs() { return false; }

uint32_t balansun_self_test_stamp_run_epoch() {
  if (time_sync_valid) {
    return static_cast<uint32_t>(time(nullptr));
  }
  return 1u;
}

void balansun_self_test_tick(unsigned long now_ms) {
  if (g_phase == Phase::Idle) {
    if (g_boot_self_test_pending && !g_self_test.skipped && boot_self_test_prerequisites_met()) {
      balansun_self_test_start_run();
    } else {
      return;
    }
  }

  if (g_phase == Phase::Idle) return;

  if (g_phase == Phase::ZcSample) {
    int in_d = 0, raw = 0;
    TriacReadAndResetCounters(in_d, raw);
    g_zc_total += in_d;
    if ((now_ms - g_phase_start_ms) >= 1000UL) {
      g_self_test.zc_edges_per_sec = (uint16_t)g_zc_total;
      const uint32_t hz = balansun_mains_effective_frequency_hz();
      const int expect = (hz >= 55) ? 110 : 90;
      const int tol = (hz >= 55) ? 25 : 20;
      g_self_test.zc_ok =
          g_self_test.zc_edges_per_sec >= (uint16_t)(expect - tol) &&
          g_self_test.zc_edges_per_sec <= (uint16_t)(expect + tol + 40);
      g_phase = Phase::TriacDry;
      g_phase_start_ms = now_ms;
    }
    return;
  }

  if (g_phase == Phase::TriacDry) {
    const BalansunTriacSyncState sync =
        balansun_triac_sync_state_from_zc(zc_sync_state, balansun_product_caps_compile_time());
    g_self_test.triac_ok = sync == BalansunTriacSyncState::Synced || zc_sync_state > 0;
    g_phase = Phase::SourceCheck;
    g_phase_start_ms = now_ms;
    return;
  }

  if (g_phase == Phase::SourceCheck) {
    g_self_test.source_ok = (last_metering_task_ms > 0) && (time_sync_valid || meter_reading_valid);
    g_self_test.run_epoch = balansun_self_test_stamp_run_epoch();
    g_phase = Phase::Done;
    persistConfigToEeprom();
    return;
  }
}

bool balansun_self_test_is_running() {
  return g_phase == Phase::ZcSample || g_phase == Phase::TriacDry || g_phase == Phase::SourceCheck;
}

void balansun_self_test_append_health_json(JsonObject obj) {
  obj["pending"] = g_self_test.pending;
  obj["skipped"] = g_self_test.skipped;
  obj["running"] = balansun_self_test_is_running();
  if (g_boot_self_test_pending && !g_self_test.skipped && g_phase == Phase::Idle) {
    obj["phase"] = "waiting_sync";
  } else {
    obj["phase"] = phase_wire(g_phase);
  }
  obj["last_run_epoch"] = g_self_test.run_epoch;
  JsonObject results = obj["results"].to<JsonObject>();
  results["zc_ok"] = g_self_test.zc_ok;
  results["triac_ok"] = g_self_test.triac_ok;
  results["source_ok"] = g_self_test.source_ok;
  results["zc_edges_per_sec"] = g_self_test.zc_edges_per_sec;

  const BalansunSelfTestSafetyResult safety = balansun_self_test_safety_eval_now();
  const bool stale_guard = triacOffWhenSourceStale;
  JsonObject severity = obj["severity"].to<JsonObject>();
  severity["zc"] = balansun_self_test_severity_wire(g_self_test.zc_ok, true);
  severity["triac"] = balansun_self_test_severity_wire(g_self_test.triac_ok, true);
  severity["source"] =
      balansun_self_test_severity_wire(g_self_test.source_ok, stale_guard);
  obj["safety_lockout_active"] = false;
  const bool critical_warning =
      g_self_test.run_epoch != 0 && !g_self_test.skipped &&
      balansun_self_test_safety_has_critical_failure(safety);
  obj["critical_warning_active"] = critical_warning;
  JsonArray reasons = obj["safety_lockout_reasons"].to<JsonArray>();
  const char *reason_wires[3];
  size_t reason_count = 0;
  balansun_self_test_safety_lockout_reasons(safety, reason_wires, 3, &reason_count);
  for (size_t i = 0; i < reason_count; i++) {
    reasons.add(reason_wires[i]);
  }
}
