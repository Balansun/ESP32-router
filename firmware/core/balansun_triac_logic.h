#pragma once

#include <balansun/triac.h>

#define TRIAC_LOGIC_IRAM BALANSUN_TRIAC_LOGIC_IRAM

inline int TRIAC_LOGIC_IRAM triac_delay_threshold_ticks(int triac_delay_percent, int max_delay_ticks) {
  return balansun_triac_delay_threshold_ticks(triac_delay_percent, max_delay_ticks);
}

inline uint8_t triac_max_delay_ticks_from_half_period_us(uint32_t half_period_us) {
  return balansun_triac_max_delay_ticks_from_half_period_us(half_period_us);
}
