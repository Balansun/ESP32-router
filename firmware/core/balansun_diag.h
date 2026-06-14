#pragma once

#include "balansun_regulation_hunting_logic.h"
#include "balansun_triac_calibration_logic.h"
#include "uxi_adc_clip_logic.h"

#include <Arduino.h>

extern TriacCalibrationTable g_triac_cal;

extern UxiAdcClipState g_uxi_adc_clip;
extern uint8_t g_source_error_streak;
extern RegulationHuntingState g_regulation_hunting_state;
extern RegulationHuntingConfig g_regulation_hunting_config;
extern bool g_regulation_hunting_active;

bool balansun_diag_uxi_adc_clipping_active();
void balansun_diag_on_source_poll_result(bool ok);
void balansun_diag_regulation_hunting_tick(unsigned long now_ms, int triac_open_percent);
