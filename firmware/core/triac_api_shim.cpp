#include "triac_api_shim.h"
#include "balansun_triac_isr.h"

void TriacReadAndResetCounters(int &inDeglitch, int &raw) {
  noInterrupts();
  raw = IT_half_period;
  inDeglitch = IT_half_period_in;
  IT_half_period = 0;
  IT_half_period_in = 0;
  interrupts();
}

int TriacGetOpenPercent() {
  return 100 - triac_delay_percent;
}
