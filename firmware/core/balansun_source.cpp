/*
 * balansun_source.cpp — Poll dispatch to IMeterDriver instances, HA wire labels, JSON diagnostics.
 */
#include "balansun_source.h"
#include "balansun_meter_registry.h"
#include "balansun_meter_driver_iface.h"
#include "balansun_meter_sources_enable.h"
#include "balansun_source_logic.h"
#include "balansun_source_health_logic.h"
#include "balansun_diag.h"
#include "balansun_forward.h"
#include "balansun_globals.h"
#include "balansun_mains_profile.h"
#include "api_util.h"
#include <ArduinoJson.h>

#if BALANSUN_ENABLE_SOURCE_JSY_MK333
#include "jsy_mk333_meter.h"
#endif

static SourceId g_active_source = SourceId::Unknown;

static SourceId parse_wire(const String &w) { return balansun_source_logic_parse_wire(w.c_str()); }

void balansun_active_source_refresh_from_global_string() {
  g_active_source = parse_wire(String(Source.c_str()));
}

SourceId balansun_active_source_get() { return g_active_source; }

SourceId balansun_effective_meter_id() {
  return balansun_source_logic_effective_id(g_active_source, Source_data.c_str());
}

size_t balansun_source_registry_count() { return balansun_meter_instance_count(); }

const char *balansun_source_wire_at(size_t i) {
  IMeterDriver *d = balansun_meter_instance_at(i);
  return d ? d->wire() : "";
}

const char *balansun_source_wire_for_id(SourceId id) {
  IMeterDriver *d = balansun_meter_driver_for_id(id);
  return d ? d->wire() : "";
}

bool balansun_source_wire_supported(const String &wire) { return parse_wire(wire) != SourceId::Unknown; }

void balansun_source_apply_hardware_setup() {
  balansun_active_source_refresh_from_global_string();
  IMeterDriver *row = balansun_meter_driver_for_id(g_active_source);
  if (row) {
    row->setup();
  }
#if BALANSUN_ENABLE_SOURCE_JSY_MK333
  if (g_active_source == SourceId::JsyMk333) {
    delay(100);
    jsy_mk333_send_request();
    poll_period_ms = balansun_source_logic_base_poll_period_ms(SourceId::JsyMk333, JsyMk333SerialBaud);
  }
#endif
  if (g_active_source != SourceId::BalansunPeer) {
    Source_data = Source;
  }
}

void balansun_source_run_poll_cycle(unsigned long pollBackoffMs) {
  IMeterDriver *row = balansun_meter_driver_for_id(g_active_source);
  if (!row) {
    poll_period_ms = 1000;
    return;
  }
  row->poll();
  if (row->flags() & BALANSUN_METER_RSF_TOUCH_LAST_MS) {
    last_metering_task_ms = millis();
  }
  uint32_t p = balansun_source_logic_base_poll_period_ms(g_active_source, JsyMk333SerialBaud);
  if (row->flags() & BALANSUN_METER_RSF_POLL_BACKOFF) {
    p += (uint32_t)pollBackoffMs;
  }
  poll_period_ms = p;
  if (mains_frequency_hz >= 45.0f && mains_frequency_hz <= 65.0f) {
    balansun_mains_on_meter_frequency(mains_frequency_hz);
  }
  bool poll_ok = true;
  if (g_active_source == SourceId::BalansunPeer) {
    poll_ok = ext_peer_last_poll_ok;
  } else {
    poll_ok = time_sync_valid;
  }
  balansun_diag_on_source_poll_result(poll_ok);
}

namespace {

BalansunSourceHealthPollState balansun_source_health_poll_state_now(void) {
  BalansunSourceHealthPollState st{};
  st.active_source = balansun_active_source_get();
  st.ext_peer_last_poll_ok = ext_peer_last_poll_ok;
  st.time_sync_valid = time_sync_valid;
  if (last_metering_task_ms > 0) {
    st.last_poll_ms_ago = static_cast<int>((millis() - last_metering_task_ms) & 0x7FFFFFFF);
  }
  st.poll_period_ms = poll_period_ms;
  st.error_streak = g_source_error_streak;
  return st;
}

}  // namespace

void balansun_source_health_input_now(SourceHealthScoreInput &out) {
  balansun_source_health_input_from_poll_state(balansun_source_health_poll_state_now(), out);
}

int balansun_source_health_score_now(void) {
  return balansun_source_health_score_from_poll_state(balansun_source_health_poll_state_now());
}

bool balansun_source_is_stale_now(void) {
  return balansun_source_is_stale_from_poll_state(balansun_source_health_poll_state_now());
}

void balansun_sources_diagnostics_append_summary(JsonObject root) {
  JsonObject diag = root["diagnostics"].to<JsonObject>();
  const SourceId active = balansun_active_source_get();
  const SourceId eff = balansun_effective_meter_id();
  diag["active_source"] = balansun_source_wire_for_id(active);
  diag["meter"] = balansun_source_wire_for_id(eff);
  if (active == SourceId::BalansunPeer && Source_data.length() > 0) {
    diag["source_data"] = Source_data;
  }
  diag["poll_period_ms"] = poll_period_ms;
  if (last_metering_task_ms > 0) {
    diag["last_poll_ms_ago"] = (int)((millis() - last_metering_task_ms) & 0x7FFFFFFF);
  } else {
    diag["last_poll_ms_ago"] = -1;
  }
  if (active == SourceId::BalansunPeer) {
    diag["last_poll_ok"] = ext_peer_last_poll_ok;
    diag["last_error"] = ext_peer_last_poll_err;
    diag["protocol_used"] = ext_peer_last_poll_protocol.length() ? ext_peer_last_poll_protocol : "json";
    diag["ext_protocol"] = ext_peer_protocol.length() ? ext_peer_protocol : "json";
    if (ext_peer_last_poll_ms > 0) {
      diag["transport_last_poll_ms_ago"] =
          (int)((millis() - ext_peer_last_poll_ms) & 0x7FFFFFFF);
    } else {
      diag["transport_last_poll_ms_ago"] = -1;
    }
  } else {
    diag["last_poll_ok"] = time_sync_valid;
    diag["last_error"] = "";
  }
  diag["mains_frequency_hz"] = balansun_mains_effective_frequency_hz();
  diag["mains_frequency_source"] = balansun_mains_frequency_source_string();
  const char *warn = balansun_mains_frequency_warning_string();
  if (warn && warn[0]) {
    diag["mains_frequency_warning"] = warn;
  }
  SourceHealthScoreInput hin;
  balansun_source_health_input_now(hin);
  const SourceHealthScoreResult score = balansun_source_health_logic_compute(hin);
  diag["health_score"] = score.health_score;
  JsonObject factors = diag["health_score_factors"].to<JsonObject>();
  factors["freshness"] = score.freshness_pts;
  factors["poll_ok"] = score.poll_ok_pts;
  factors["streak"] = score.streak_pts;
  if (balansun_diag_uxi_adc_clipping_active()) {
    diag["adc_clipping"] = true;
  }
  if (g_regulation_hunting_active) {
    diag["regulation_hunting"] = true;
  }
}

void balansun_sources_diagnostics_append_meter_payload(JsonObject doc, int linky_tail_max) {
  const SourceId eff = balansun_effective_meter_id();
  IMeterDriver *eff_driver = balansun_meter_driver_for_id(eff);
  if (eff_driver) {
    eff_driver->appendDiagnostics(doc, linky_tail_max);
  }
  const SourceId active = balansun_active_source_get();
  if (active != eff) {
    IMeterDriver *active_driver = balansun_meter_driver_for_id(active);
    if (active_driver) {
      active_driver->appendDiagnostics(doc, linky_tail_max);
    }
  }
}

void balansun_sources_brute_panel_json(String &out) {
  JsonDocument d;
  const SourceId eff = balansun_effective_meter_id();
  d["panel"] = balansun_source_wire_for_id(eff);
  JsonObject p = d["panels"].to<JsonObject>();
  p["analog"] = (eff == SourceId::Analog);
  p["jsy_mk194"] = (eff == SourceId::JsyMk194);
  p["jsy_mk333"] = (eff == SourceId::JsyMk333);
  p["linky"] = (eff == SourceId::Linky);
  p["enphase"] = (eff == SourceId::Enphase);
  p["smartg"] = (eff == SourceId::SmartG);
  p["shelly_em"] = (eff == SourceId::ShellyEm);
  p["homewizard"] = (eff == SourceId::HomeW);
  p["shelly_pro"] = (eff == SourceId::ShellyPro);
  p["pmqtt"] = (eff == SourceId::Pmqtt);
  p["notdef"] = (eff == SourceId::NotDef);
  serializeJson(d, out);
}

bool balansun_cap_mqtt_triac_channel_block_for(SourceId id) {
  return balansun_source_logic_cap_mqtt_triac_channel_block_for(id);
}

bool balansun_cap_mqtt_triac_channel_block() {
  return balansun_cap_mqtt_triac_channel_block_for(g_active_source);
}

bool balansun_cap_mqtt_linky_tariff_for(SourceId id) {
  return balansun_source_logic_cap_mqtt_linky_tariff_for(id, tempoRteEnabled);
}

bool balansun_cap_mqtt_linky_tariff() { return balansun_cap_mqtt_linky_tariff_for(g_active_source); }

bool balansun_cap_serial_adc_gpio_restrict_for(SourceId id) {
  return balansun_source_logic_cap_serial_adc_gpio_restrict_for(id);
}

bool balansun_cap_serial_adc_gpio_restrict() {
  return balansun_cap_serial_adc_gpio_restrict_for(g_active_source);
}
