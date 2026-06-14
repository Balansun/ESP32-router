#pragma once

/*
 * Multi-sinus / train / demi-sinus tables and 10 ms half-wave stepping.
 */

#include <Arduino.h>
#include "balansun_board.h"
#include "balansun_regulation_modes.h"

#include <cstdint>

/** Build tabPulseSinusOn/Total (call once at boot). */
void balansun_pulse_modes_init_tables(void);

/** After regulation: set PulseOn/PulseTotal for multi/train/demi modes. */
void balansun_pulse_modes_apply_regulation_output(int action_index, uint8_t regulation_mode, int triac_delay_percent_percent);

/** 10 ms / ZC-sync tick — GPIO pulse patterns (ISR-safe). */
#if defined(FLEET_BUNDLE_NATIVE_STUB)
void balansun_pulse_modes_tick_10ms(void);
#else
void IRAM_ATTR balansun_pulse_modes_tick_10ms(void);
#endif

uint8_t balansun_pulse_sinus_on(uint8_t open_percent);
uint8_t balansun_pulse_sinus_total(uint8_t open_percent);
