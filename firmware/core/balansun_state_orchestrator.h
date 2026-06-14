#pragma once

#include "balansun_product_caps.h"

/** Inputs for one operational state tick (L0–L3 + triac sync). */
struct BalansunStateTickInput {
  BalansunProductCaps caps{};
  bool api_telemetry_ready = false;
  bool vacation_active = false;
  bool source_stale = false;
  bool triac_off_when_source_stale = false;
  bool self_test_running = false;
  int triac_open_percent = 0;
  bool schedule_allows_surplus = false;
  int ceiling_percent = 100;
};

/** Aggregate L1/L0/L3/triac-sync into g_balansun_state_snapshot (cap-gated per layer). */
void balansun_state_orchestrator_tick(const BalansunStateTickInput &in);

/** Refresh snapshot from live globals (meter-only loop, HTTP handlers). */
void balansun_state_orchestrator_tick_runtime(int triac_schedule_type = 0, int ceiling_percent = 100,
                                           int triac_open_percent = -1);

/** Safe before serializing /health or /state (avoids stale cross-core reads). */
void balansun_state_orchestrator_tick_for_http(void);
