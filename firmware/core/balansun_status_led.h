#pragma once

#include "balansun_status_led_logic.h"

#include <ArduinoJson.h>
#include <WString.h>

void balansun_status_led_init(void);
void balansun_status_led_reinit(void);
void balansun_status_led_apply_live(int cpt_activity, int cpt_regulation);
void balansun_status_led_apply_overlay(StatusLedOverlay overlay, unsigned tick);
bool balansun_status_led_start_test(StatusLedTestRole role, unsigned duration_ms, const StatusLedConfig *override_cfg);
void balansun_status_led_sync_globals_from_config(void);
void balansun_status_led_load_globals_into_config(void);
bool balansun_status_led_merge_json(JsonObjectConst root, StatusLedConfig &cfg, String &err);
bool balansun_status_led_apply_config_json(JsonObjectConst root, String &err);
bool balansun_status_led_build_test_config(JsonObjectConst root, StatusLedConfig &cfg, String &err);

StatusLedConfig balansun_status_led_config_now(void);
StatusLedBoardPins balansun_status_led_board_pins(void);
const char *balansun_status_led_mode_string(StatusLedMode mode);
bool balansun_status_led_mode_from_string(const char *s, StatusLedMode &out);
