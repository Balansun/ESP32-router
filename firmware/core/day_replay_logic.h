#pragma once

#include "actions_logic.h"
#include "balansun_regulation_logic.h"

#include <cstddef>
#include <cstdint>
#include <vector>

struct DayReplaySlot {
  int wall_decihours = 0;
  int active_import_w = 0;
  int active_export_w = 0;
  float temperature_c = 20.0f;
  int loops = 15;
};

struct DayReplayConfig {
  ActionScheduleConfig schedule;
  uint8_t actif = 1;
  int loop_gain = 4;
  int triac_max_percent = 100;
  float initial_triac_delay_percent_f = 100.0f;
};

struct DayReplaySample {
  int slot_index = 0;
  int net_power_w = 0;
  float triac_delay_percent_f = 100.0f;
  int triac_open_percent = 0;
  uint8_t schedule_type_triac = 0;
};

struct DayReplayCheckpoint {
  int slot_index = 0;
  int triac_open_percent_min = 0;
  int triac_open_percent_max = 100;
  bool schedule_active = true;
};

int day_replay_house_net_w(int active_import_w, int active_export_w);
int day_replay_triac_open_percent(float triac_delay_percent_f);

std::vector<DayReplaySample> day_replay_run(const DayReplayConfig &cfg, const DayReplaySlot *slots, size_t n);

/** Returns -1 if all checkpoints pass, else first failing slot_index. */
int day_replay_check_checkpoints(const std::vector<DayReplaySample> &samples,
                                 const DayReplayCheckpoint *checkpoints, size_t n);
