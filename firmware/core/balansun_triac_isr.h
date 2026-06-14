#pragma once

/*
 * balansun_triac_isr.h — Volatile triac timing state shared between ISRs and main loop.
 */

#include <Arduino.h>



/** Last accepted zero-cross time (`esp_timer_get_time()`, µs) for ISR debounce. */

extern volatile uint32_t last_zc_us;

extern volatile int phase_delay_ticks;

extern volatile int triac_delay_percent;

/** zc_sync_state: >0 ZC-synced, <0 internal 10 ms clock. */

extern volatile int16_t zc_sync_state;

extern volatile int IT_half_period;

extern volatile int IT_half_period_in;

extern hw_timer_t *timer;

extern hw_timer_t *timer10ms;



void balansun_zc_isr();

void balansun_phase_tick_isr();

void balansun_half_cycle_tick_isr();



/** Zero-cross + 100 µs + 10 ms timers. */

void balansun_triac_hw_init(void);

void balansun_triac_refresh_dim_gpio_mask(void);

/** Attach ZC ISR once mains edges are seen (avoids floating-pin IRQ storms on bench). */
void balansun_triac_enable_zc_interrupt(void);

/** Arm the 100 µs phase timer after the first zero-cross (call from main loop). */

void balansun_triac_poll_hw(void);

/** Action 0 regulation mode for ISR (MODE_DECOUPE_ONOFF). */

void balansun_triac_set_action0_mode(uint8_t mode);

/** ISR-safe triac DIM line (GPIO kTriacDimGpio); use instead of digitalWrite from IRAM paths. */

void IRAM_ATTR balansun_triac_set_dim_level_isr(int level);
