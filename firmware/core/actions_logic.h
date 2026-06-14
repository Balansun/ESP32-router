#pragma once

/* actions_logic.h — Schedule/override evaluation for routing actions (host-testable). */

#include <cstdint>

constexpr uint8_t kActionOverrideAuto = 0;
constexpr uint8_t kActionOverrideOn = 1;
constexpr uint8_t kActionOverrideOff = 2;
constexpr uint8_t kActionOverrideTriacFixed = 3;

struct ActionSchedulePeriod {
  uint8_t type = 0;
  int period_start = 0;
  int period_end = 0;
  int power_min = 0;
  int power_max = 0;
  int temp_min = 0;
  int temp_max = 0;
};

struct ActionScheduleConfig {
  uint8_t period_count = 0;
  ActionSchedulePeriod periods[8];
};

uint8_t actions_logic_active_type(const ActionScheduleConfig &cfg, int wall_decihours);
uint8_t actions_logic_active_type_triac(const ActionScheduleConfig &cfg, int wall_decihours, float temperature);
int actions_logic_threshold_min(const ActionScheduleConfig &cfg, int wall_decihours);
int actions_logic_threshold_max(const ActionScheduleConfig &cfg, int wall_decihours);

bool actions_logic_override_expired(uint8_t override_state, unsigned long override_until_ms, unsigned long now_ms);

constexpr int kActionMaxPeriods = 8;
constexpr int kActionScheduleTimeMax = 2400;

int actions_logic_clamp_period_count(int count);
bool actions_logic_period_schedule_valid(uint8_t period_count, const int period_end[8],
                                       const uint8_t type[8]);
