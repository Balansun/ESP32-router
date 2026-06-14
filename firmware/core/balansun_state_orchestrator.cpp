#include "balansun_state_orchestrator.h"

#include "balansun_api_ready.h"
#include "balansun_globals.h"
#include "balansun_product_caps.h"
#include "balansun_regulation_state.h"
#include "balansun_self_test.h"
#include "balansun_source.h"
#include "balansun_state_snapshot.h"
#include "balansun_vacation_logic.h"

#include <ctime>

static void balansun_gather_runtime_tick_inputs(BalansunStateTickInput &tick, int triac_schedule_type,
                                             int ceiling_percent, int triac_open_percent) {
  const uint32_t now_epoch = static_cast<uint32_t>(time(NULL));
  vacationEnabled = balansun_vacation_logic_tick_enabled(vacationEnabled, vacationEndEpoch, now_epoch);

  tick.caps = balansun_product_caps_compile_time();
  tick.api_telemetry_ready = balansun_api_telemetry_ready();
  tick.vacation_active = balansun_vacation_logic_active(vacationEnabled, vacationEndEpoch, now_epoch);
  tick.source_stale = balansun_source_is_stale_now();
  tick.triac_off_when_source_stale = triacOffWhenSourceStale;
  tick.self_test_running = balansun_self_test_is_running();
  tick.schedule_allows_surplus = triac_schedule_type > 1;
  tick.ceiling_percent = ceiling_percent;
  if (triac_open_percent < 0) {
    tick.triac_open_percent =
        balansun_product_has_cap(BalansunCap::SurplusRegulation) ? (100 - g_triac_delay_percent[0]) : 0;
  } else {
    tick.triac_open_percent = triac_open_percent;
  }
}

void balansun_state_orchestrator_tick(const BalansunStateTickInput &in) {
  balansun_state_snapshot_refresh(in.api_telemetry_ready, in.vacation_active, in.source_stale,
                               in.triac_off_when_source_stale, in.self_test_running,
                               in.triac_open_percent, in.schedule_allows_surplus, in.ceiling_percent);
}

void balansun_state_orchestrator_tick_runtime(int triac_schedule_type, int ceiling_percent,
                                           int triac_open_percent) {
  BalansunStateTickInput tick{};
  balansun_gather_runtime_tick_inputs(tick, triac_schedule_type, ceiling_percent, triac_open_percent);
  balansun_state_orchestrator_tick(tick);
}

void balansun_state_orchestrator_tick_for_http(void) {
  balansun_state_orchestrator_tick_runtime(0, 100, -1);
}
