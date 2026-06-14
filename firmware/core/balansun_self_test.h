#pragma once

#include "balansun_self_test_persisted.h"

#include <ArduinoJson.h>

extern SelfTestPersisted g_self_test;

void balansun_self_test_set_pending(bool pending);
/** Router profiles: arm boot self-test once time and telemetry are ready. */
void balansun_self_test_on_boot();
void balansun_self_test_tick(unsigned long now_ms);
void balansun_self_test_start_run();
/** True while this boot's self-test has not passed (running or not yet completed). */
bool balansun_self_test_blocks_outputs();
void balansun_self_test_skip();
/** Clears boot auto-run arm (e.g. HIL simulate completed a run for this session). */
void balansun_self_test_disarm_boot_pending();
bool balansun_self_test_is_running();
/** UTC Unix seconds when time is synced; otherwise 1 (completed, wall clock unavailable). */
uint32_t balansun_self_test_stamp_run_epoch();
void balansun_self_test_append_health_json(JsonObject obj);
