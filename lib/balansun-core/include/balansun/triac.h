#pragma once

#include <cstdint>

#if defined(ESP_PLATFORM)
#include <esp_attr.h>
#define BALANSUN_TRIAC_LOGIC_IRAM IRAM_ATTR
#else
#define BALANSUN_TRIAC_LOGIC_IRAM
#endif

/** Triac gate delay threshold in 100 µs ticks (ISR semantics). */
int BALANSUN_TRIAC_LOGIC_IRAM balansun_triac_delay_threshold_ticks(int triac_open_percent, int max_delay_ticks);

/** Compute max delay ticks from half-period microseconds. */
uint8_t balansun_triac_max_delay_ticks_from_half_period_us(uint32_t half_period_us);
