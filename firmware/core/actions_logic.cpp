#include "actions_logic.h"

uint8_t actions_logic_active_type(const ActionScheduleConfig &cfg, int wall_decihours) {
  uint8_t s = 0;
  for (int i = 0; i < cfg.period_count; i++) {
    if (wall_decihours >= cfg.periods[i].period_start && wall_decihours <= cfg.periods[i].period_end) {
      s = cfg.periods[i].type;
    }
  }
  return s;
}

uint8_t actions_logic_active_type_triac(const ActionScheduleConfig &cfg, int wall_decihours, float temperature) {
  uint8_t s = 0;
  for (int i = 0; i < cfg.period_count; i++) {
    bool temperature_ok = true;
    if (temperature > -100) {
      if (cfg.periods[i].temp_min <= 100 && temperature > cfg.periods[i].temp_min) temperature_ok = false;
      if (cfg.periods[i].temp_max <= 100 && temperature < cfg.periods[i].temp_max) temperature_ok = false;
    }
    if (wall_decihours >= cfg.periods[i].period_start && wall_decihours <= cfg.periods[i].period_end &&
        temperature_ok) {
      s = cfg.periods[i].type;
    }
  }
  return s;
}

int actions_logic_threshold_min(const ActionScheduleConfig &cfg, int wall_decihours) {
  int s = 0;
  for (int i = 0; i < cfg.period_count; i++) {
    if (wall_decihours >= cfg.periods[i].period_start && wall_decihours <= cfg.periods[i].period_end) {
      s = cfg.periods[i].power_min;
      if (cfg.periods[i].type == 2) s = 32000;
    }
  }
  return s;
}

int actions_logic_threshold_max(const ActionScheduleConfig &cfg, int wall_decihours) {
  int s = 0;
  for (int i = 0; i < cfg.period_count; i++) {
    if (wall_decihours >= cfg.periods[i].period_start && wall_decihours <= cfg.periods[i].period_end) {
      s = cfg.periods[i].power_max;
      if (cfg.periods[i].type == 2) s = 100;
    }
  }
  return s;
}

bool actions_logic_override_expired(uint8_t override_state, unsigned long override_until_ms, unsigned long now_ms) {
  return override_state != kActionOverrideAuto && override_until_ms != 0 &&
         static_cast<long>(now_ms - override_until_ms) >= 0;
}

int actions_logic_clamp_period_count(int count) {
  if (count < 1) return 1;
  if (count > kActionMaxPeriods) return kActionMaxPeriods;
  return count;
}

bool actions_logic_period_schedule_valid(uint8_t period_count, const int period_end[8],
                                         const uint8_t type[8]) {
  const int n = actions_logic_clamp_period_count(period_count);
  if (static_cast<int>(period_count) != n) return false;
  int prev_end = 0;
  for (int i = 0; i < n; i++) {
    if (type[i] < 1 || type[i] > 3) return false;
    const int end = period_end[i];
    if (end < 0 || end > kActionScheduleTimeMax) return false;
    if (end <= prev_end) return false;
    prev_end = end;
  }
  if (period_end[n - 1] != kActionScheduleTimeMax) return false;
  return true;
}
