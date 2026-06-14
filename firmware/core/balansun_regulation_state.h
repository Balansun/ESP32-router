#pragma once

/*
 * Per-action regulation RAM state (integrator, RetardF, pulse counters).
 */

#include "balansun_board.h"

#include <cstdint>

constexpr int kRegulationActions = kMaxRoutingActions;

extern float g_triac_delay_percent_f[kRegulationActions];
extern int g_triac_delay_percent[kRegulationActions];
extern float g_surplus_integrator[kRegulationActions];
extern float g_surplus_last_error[kRegulationActions];
extern float g_surplus_proportional[kRegulationActions];
extern float g_surplus_derivative[kRegulationActions];

extern uint8_t g_pulse_on[kRegulationActions];
extern uint8_t g_pulse_total[kRegulationActions];
extern int g_pulseCounter[kRegulationActions];
extern bool g_phase_230v;

/** Call once at boot (integrator starts at 100 to avoid opening at startup). */
void balansun_regulation_state_init(void);

/** Sync load channel 0 into ISR-facing triac percent globals. */
void balansun_regulation_sync_triac_globals(void);
