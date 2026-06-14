#include "balansun_diag.h"

#include "balansun_source.h"
#include "balansun_source_types.h"
#include "triac_api_shim.h"

TriacCalibrationTable g_triac_cal;
UxiAdcClipState g_uxi_adc_clip;
uint8_t g_source_error_streak = 0;
RegulationHuntingState g_regulation_hunting_state;
RegulationHuntingConfig g_regulation_hunting_config;
bool g_regulation_hunting_active = false;

bool balansun_diag_uxi_adc_clipping_active() {
  const SourceId eff = balansun_effective_meter_id();
  if (eff != SourceId::Analog && eff != SourceId::JsyMk194) {
    return false;
  }
  return uxi_adc_clip_logic_is_clipping(g_uxi_adc_clip);
}

void balansun_diag_on_source_poll_result(bool ok) {
  if (ok) {
    if (g_source_error_streak > 0) g_source_error_streak--;
    else g_source_error_streak = 0;
  } else {
    if (g_source_error_streak < 255) g_source_error_streak++;
  }
}

void balansun_diag_regulation_hunting_tick(unsigned long now_ms, int triac_open_percent) {
  balansun_regulation_hunting_logic_sample(g_regulation_hunting_state, g_regulation_hunting_config,
                                      triac_open_percent, now_ms);
  g_regulation_hunting_active = balansun_regulation_hunting_logic_is_hunting(g_regulation_hunting_state);
}
