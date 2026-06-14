#include "balansun_vacation_logic.h"

bool balansun_vacation_logic_active(bool enabled, uint32_t end_epoch_utc, uint32_t now_epoch_utc) {
  if (!enabled) return false;
  if (end_epoch_utc == 0) return true;
  if (now_epoch_utc < 1000) return true;
  return now_epoch_utc < end_epoch_utc;
}

bool balansun_vacation_logic_tick_enabled(bool enabled, uint32_t end_epoch_utc, uint32_t now_epoch_utc) {
  if (!enabled) return false;
  if (end_epoch_utc == 0 || now_epoch_utc < 1000) return enabled;
  if (now_epoch_utc >= end_epoch_utc) return false;
  return true;
}
