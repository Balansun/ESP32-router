#include "balansun_pulse_modes.h"

#include "Actions.h"
#include "balansun_globals.h"
#include "balansun_regulation_state.h"
#include "balansun_triac_isr.h"

#include <Arduino.h>

void balansun_pulse_modes_apply_regulation_output(int action_index, uint8_t regulation_mode, int triac_delay_percent_percent) {
  if (action_index < 0 || action_index >= kRegulationActions) return;
  const int open_percent = 100 - triac_delay_percent_percent;
  switch (regulation_mode) {
    case kModeMultisinus:
      g_pulse_on[action_index] = balansun_pulse_sinus_on(static_cast<uint8_t>(open_percent));
      g_pulse_total[action_index] = balansun_pulse_sinus_total(static_cast<uint8_t>(open_percent));
      if (g_pulseCounter[action_index] >= g_pulse_total[action_index]) {
        g_pulseCounter[action_index] = 0;
      }
      break;
    case kModeTrainsinus:
      g_pulse_on[action_index] = static_cast<uint8_t>(open_percent);
      g_pulse_total[action_index] = 99;
      break;
    case kModeDemisinus:
      g_pulse_on[action_index] = static_cast<uint8_t>(open_percent);
      if (g_pulse_total[action_index] > 1) g_pulse_total[action_index] = 0;
      break;
    default:
      break;
  }
}

void IRAM_ATTR balansun_pulse_modes_tick_10ms(void) {
  g_phase_230v = !g_phase_230v;
  extern int NbActions;
  for (int i = 0; i < NbActions && i < kRegulationActions; i++) {
    Action &act = load_channels[i];
    const uint8_t mode = act.Actif;
    switch (mode) {
      case kModeInactif:
        break;
      case kModeDecoupeOnoff:
        if (i == 0) {
          phase_delay_ticks = 0;
          balansun_triac_set_dim_level_isr(0);
        } else if (g_triac_delay_percent[i] < 100) {
          act.relay_on_from_triac_delay(g_triac_delay_percent[i]);
        }
        break;
      case kModePwm:
        act.apply_pwm_from_triac_delay_f(g_triac_delay_percent_f[i]);
        break;
      case kModeDemisinus:
        act.tick_half_sine(g_phase_230v, g_pulse_on[i], g_pulse_total[i], g_pulseCounter[i]);
        break;
      default:
        act.tick_pulse_train(g_pulse_on[i], g_pulse_total[i], g_pulseCounter[i]);
        break;
    }
  }
}
