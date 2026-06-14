#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>

#include "day_replay_logic.h"
#include "balansun_regulation_modes.h"

TEST(DayReplayLogic, MiddayExportOpensTriac) {
  DayReplayConfig cfg;
  cfg.schedule.period_count = 1;
  cfg.schedule.periods[0] = {4, 0, 2400, 0, 80, 101, 101};
  cfg.triac_max_percent = 80;
  DayReplaySlot sl;
  sl.wall_decihours = 1200;
  sl.active_import_w = 50;
  sl.active_export_w = 3500;
  sl.loops = 20;
  const auto out = day_replay_run(cfg, &sl, 1);
  ASSERT_EQ(out.size(), 1u);
  EXPECT_GT(out[0].triac_open_percent, 5);
}

TEST(DayReplayLogic, ImportAfterExportClosesTriac) {
  DayReplayConfig cfg;
  cfg.schedule.period_count = 1;
  cfg.schedule.periods[0] = {4, 0, 2400, 0, 80, 101, 101};
  cfg.initial_triac_delay_percent_f = 30.0f;
  DayReplaySlot export_sl;
  export_sl.wall_decihours = 1200;
  export_sl.active_import_w = 0;
  export_sl.active_export_w = 4000;
  export_sl.loops = 25;
  DayReplaySlot import_sl;
  import_sl.wall_decihours = 1200;
  import_sl.active_import_w = 5000;
  import_sl.active_export_w = 0;
  import_sl.loops = 25;
  DayReplaySlot slots[2] = {export_sl, import_sl};
  const auto out = day_replay_run(cfg, slots, 2);
  ASSERT_EQ(out.size(), 2u);
  EXPECT_GT(out[0].triac_open_percent, out[1].triac_open_percent);
}

TEST(DayReplayLogic, RetardFStaysBoundedOnCloudyProfile) {
  std::ifstream in("firmware/test/fixtures/days/cloudy_day.json");
  ASSERT_TRUE(in.good());
  std::stringstream ss;
  ss << in.rdbuf();
  const std::string blob = ss.str();
  DayReplayConfig cfg;
  cfg.schedule.period_count = 1;
  cfg.schedule.periods[0] = {4, 0, 2400, 0, 80, 101, 101};
  std::vector<DayReplaySlot> slots;
  size_t pos = 0;
  while ((pos = blob.find("\"wall_decihours\"", pos)) != std::string::npos) {
    DayReplaySlot sl;
    const size_t hs = blob.find(':', pos);
    sl.wall_decihours = std::atoi(blob.c_str() + hs + 1);
    const size_t ss_pos = blob.find("\"active_import_w\"", pos);
    sl.active_import_w = std::atoi(blob.c_str() + blob.find(':', ss_pos) + 1);
    const size_t si_pos = blob.find("\"active_export_w\"", pos);
    sl.active_export_w = std::atoi(blob.c_str() + blob.find(':', si_pos) + 1);
    sl.loops = 8;
    slots.push_back(sl);
    pos = hs + 1;
    if (slots.size() >= 12) break;
  }
  ASSERT_GE(slots.size(), 8u);
  const auto out = day_replay_run(cfg, slots.data(), slots.size());
  for (const auto &s : out) {
    EXPECT_GE(s.triac_delay_percent_f, 0.0f);
    EXPECT_LE(s.triac_delay_percent_f, 100.0f);
  }
}

TEST(DayReplayLogic, EmptyInputReturnsEmpty) {
  DayReplayConfig cfg;
  EXPECT_TRUE(day_replay_run(cfg, nullptr, 0).empty());
  DayReplaySlot sl;
  EXPECT_TRUE(day_replay_run(cfg, &sl, 0).empty());
}

TEST(DayReplayLogic, CheckpointsDetectFailures) {
  DayReplayConfig cfg;
  cfg.schedule.period_count = 1;
  cfg.schedule.periods[0] = {4, 0, 2400, 0, 80, 101, 101};
  DayReplaySlot sl;
  sl.wall_decihours = 1200;
  sl.active_import_w = 0;
  sl.active_export_w = 4000;
  sl.loops = 5;
  const auto samples = day_replay_run(cfg, &sl, 1);
  ASSERT_EQ(samples.size(), 1u);

  DayReplayCheckpoint bad_idx;
  bad_idx.slot_index = 9;
  EXPECT_EQ(day_replay_check_checkpoints(samples, &bad_idx, 1), 9);

  DayReplayCheckpoint negative_idx;
  negative_idx.slot_index = -1;
  EXPECT_EQ(day_replay_check_checkpoints(samples, &negative_idx, 1), -1);

  DayReplayCheckpoint schedule_off;
  schedule_off.slot_index = 0;
  schedule_off.schedule_active = true;
  schedule_off.triac_open_percent_min = 0;
  schedule_off.triac_open_percent_max = 100;
  cfg.schedule.periods[0].type = 0;
  const auto inactive = day_replay_run(cfg, &sl, 1);
  EXPECT_EQ(day_replay_check_checkpoints(inactive, &schedule_off, 1), 0);

  DayReplayCheckpoint too_high;
  too_high.slot_index = 0;
  too_high.triac_open_percent_min = 0;
  too_high.triac_open_percent_max = 0;
  EXPECT_EQ(day_replay_check_checkpoints(samples, &too_high, 1), 0);

  DayReplayCheckpoint above_max;
  above_max.slot_index = 0;
  above_max.triac_open_percent_min = 0;
  above_max.triac_open_percent_max = 1;
  EXPECT_EQ(day_replay_check_checkpoints(samples, &above_max, 1), 0);
  EXPECT_EQ(day_replay_check_checkpoints(samples, nullptr, 1), -1);

  DayReplayCheckpoint ok;
  ok.slot_index = 0;
  ok.triac_open_percent_min = 0;
  ok.triac_open_percent_max = 100;
  EXPECT_EQ(day_replay_check_checkpoints(samples, &ok, 1), -1);
}

TEST(DayReplayLogic, HouseNetAndTriacOpenPercent) {
  EXPECT_EQ(day_replay_house_net_w(100, 40), 60);
  EXPECT_EQ(day_replay_triac_open_percent(65.0f), 35);
}

TEST(DayReplayLogic, DefaultActifAndSingleLoop) {
  DayReplayConfig cfg;
  cfg.actif = 0;
  cfg.loop_gain = 0;
  cfg.schedule.period_count = 1;
  cfg.schedule.periods[0] = {4, 0, 2400, 0, 80, 101, 101};
  DayReplaySlot sl;
  sl.wall_decihours = 1200;
  sl.active_import_w = 0;
  sl.active_export_w = 3500;
  sl.loops = 0;
  const auto out = day_replay_run(cfg, &sl, 1);
  ASSERT_EQ(out.size(), 1u);
  EXPECT_GT(out[0].triac_open_percent, 0);
}

TEST(DayReplayLogic, CheckpointFailsWhenOpenAboveMax) {
  std::vector<DayReplaySample> samples(1);
  samples[0].slot_index = 0;
  samples[0].triac_open_percent = 50;
  samples[0].schedule_type_triac = 4;
  DayReplayCheckpoint cp;
  cp.slot_index = 0;
  cp.triac_open_percent_min = 0;
  cp.triac_open_percent_max = 10;
  EXPECT_EQ(day_replay_check_checkpoints(samples, &cp, 1), 0);
}

TEST(DayReplayLogic, CheckpointFailsWhenOpenBelowMin) {
  std::vector<DayReplaySample> samples(1);
  samples[0].slot_index = 0;
  samples[0].triac_open_percent = 5;
  samples[0].schedule_type_triac = 4;
  DayReplayCheckpoint cp;
  cp.slot_index = 0;
  cp.triac_open_percent_min = 20;
  cp.triac_open_percent_max = 100;
  EXPECT_EQ(day_replay_check_checkpoints(samples, &cp, 1), 0);
}

TEST(DayReplayLogic, CheckpointFailsWhenOpenPercentTooHigh) {
  DayReplayConfig cfg;
  cfg.schedule.period_count = 1;
  cfg.schedule.periods[0] = {4, 0, 2400, 0, 80, 101, 101};
  DayReplaySlot sl;
  sl.wall_decihours = 1200;
  sl.active_import_w = 0;
  sl.active_export_w = 4000;
  sl.loops = 40;
  const auto samples = day_replay_run(cfg, &sl, 1);
  ASSERT_EQ(samples.size(), 1u);
  ASSERT_GT(samples[0].triac_open_percent, 5);
  DayReplayCheckpoint cp;
  cp.slot_index = 0;
  cp.triac_open_percent_min = 0;
  cp.triac_open_percent_max = 1;
  EXPECT_EQ(day_replay_check_checkpoints(samples, &cp, 1), 0);
}

TEST(DayReplayLogic, ColdTemperatureBlocksSchedule) {
  DayReplayConfig cfg;
  cfg.schedule.period_count = 1;
  cfg.schedule.periods[0] = {4, 0, 2400, 0, 80, 20, 40};
  DayReplaySlot sl;
  sl.wall_decihours = 1200;
  sl.temperature_c = 10.0f;
  sl.active_import_w = 0;
  sl.active_export_w = 4000;
  sl.loops = 5;
  const auto out = day_replay_run(cfg, &sl, 1);
  ASSERT_EQ(out.size(), 1u);
  EXPECT_LT(out[0].schedule_type_triac, 2u);
  EXPECT_EQ(out[0].triac_open_percent, 0);
}

TEST(DayReplayLogic, CheckpointFailsWhenScheduleInactiveExpected) {
  DayReplayConfig cfg;
  cfg.schedule.period_count = 1;
  cfg.schedule.periods[0] = {4, 0, 2400, 0, 80, 101, 101};
  DayReplaySlot sl;
  sl.wall_decihours = 1200;
  sl.active_export_w = 4000;
  sl.loops = 8;
  const auto samples = day_replay_run(cfg, &sl, 1);
  DayReplayCheckpoint cp;
  cp.slot_index = 0;
  cp.schedule_active = true;
  cp.triac_open_percent_min = 0;
  cp.triac_open_percent_max = 0;
  EXPECT_EQ(day_replay_check_checkpoints(samples, &cp, 1), 0);
}

TEST(DayReplayLogic, UsesConfiguredActifMode) {
  DayReplayConfig cfg;
  cfg.actif = kModeMultisinus;
  cfg.schedule.period_count = 1;
  cfg.schedule.periods[0] = {4, 0, 2400, 0, 80, 101, 101};
  DayReplaySlot sl;
  sl.wall_decihours = 1200;
  sl.active_import_w = 0;
  sl.active_export_w = 3500;
  sl.loops = 3;
  const auto out = day_replay_run(cfg, &sl, 1);
  ASSERT_EQ(out.size(), 1u);
  EXPECT_GT(out[0].triac_open_percent, 0);
}
