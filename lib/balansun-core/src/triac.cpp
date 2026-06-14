#include <balansun/triac.h>

int balansun_triac_delay_threshold_ticks(int triac_open_percent, int max_delay_ticks) {
  if (triac_open_percent >= 100) return max_delay_ticks + 1;
  if (triac_open_percent <= 0) return 0;
  return (triac_open_percent * max_delay_ticks) / 100;
}

uint8_t balansun_triac_max_delay_ticks_from_half_period_us(uint32_t half_period_us) {
  uint8_t max_delay = static_cast<uint8_t>(half_period_us / 100UL);
  if (max_delay > 2) {
    max_delay -= 2;
  } else {
    max_delay = 1;
  }
  return max_delay;
}
