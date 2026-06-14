#include "balansun_regulation_state.h"

float g_triac_delay_percent_f[kRegulationActions];
int g_triac_delay_percent[kRegulationActions];
float g_surplus_integrator[kRegulationActions];
float g_surplus_last_error[kRegulationActions];
float g_surplus_proportional[kRegulationActions];
float g_surplus_derivative[kRegulationActions];

uint8_t g_pulse_on[kRegulationActions];
uint8_t g_pulse_total[kRegulationActions];
int g_pulseCounter[kRegulationActions];
bool g_phase_230v = false;

void balansun_regulation_state_init(void) {
  for (int i = 0; i < kRegulationActions; i++) {
    g_triac_delay_percent_f[i] = 100.0f;
    g_triac_delay_percent[i] = 100;
    g_surplus_last_error[i] = 0.0f;
    g_surplus_integrator[i] = 100.0f;
    g_surplus_proportional[i] = 0.0f;
    g_surplus_derivative[i] = 0.0f;
    g_pulse_on[i] = 0;
    g_pulse_total[i] = 100;
    g_pulseCounter[i] = 0;
  }
}

void balansun_regulation_sync_triac_globals(void) {
  extern float triac_delay_percent_f;
  extern int triac_delay_percent;
  triac_delay_percent_f = g_triac_delay_percent_f[0];
  triac_delay_percent = g_triac_delay_percent[0];
}
