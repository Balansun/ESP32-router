#include <gtest/gtest.h>

#include "actions_logic.h"

TEST(ActionsScheduleLogic, ClampPeriodCount) {
  EXPECT_EQ(actions_logic_clamp_period_count(0), 1);
  EXPECT_EQ(actions_logic_clamp_period_count(1), 1);
  EXPECT_EQ(actions_logic_clamp_period_count(8), 8);
  EXPECT_EQ(actions_logic_clamp_period_count(200), 8);
}

TEST(ActionsScheduleLogic, ValidFactoryTriacSchedule) {
  int ends[8] = {2400, 0, 0, 0, 0, 0, 0, 0};
  uint8_t types[8] = {3, 0, 0, 0, 0, 0, 0, 0};
  EXPECT_TRUE(actions_logic_period_schedule_valid(1, ends, types));
}

TEST(ActionsScheduleLogic, RejectsGarbageCountAndEnds) {
  int ends[8] = {100, 50, 0, 0, 0, 0, 0, 0};
  uint8_t types[8] = {3, 3, 0, 0, 0, 0, 0, 0};
  EXPECT_FALSE(actions_logic_period_schedule_valid(200, ends, types));
  EXPECT_FALSE(actions_logic_period_schedule_valid(2, ends, types));
  ends[0] = -1;
  EXPECT_FALSE(actions_logic_period_schedule_valid(1, ends, types));
}

TEST(ActionsScheduleLogic, RejectsInvalidTypeAndEndNot2400) {
  int ends[8] = {2400, 0, 0, 0, 0, 0, 0, 0};
  uint8_t types[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  EXPECT_FALSE(actions_logic_period_schedule_valid(1, ends, types));
  types[0] = 4;
  EXPECT_FALSE(actions_logic_period_schedule_valid(1, ends, types));
  types[0] = 3;
  ends[0] = 2399;
  EXPECT_FALSE(actions_logic_period_schedule_valid(1, ends, types));
  ends[0] = 100;
  ends[1] = 50;
  types[1] = 2;
  EXPECT_FALSE(actions_logic_period_schedule_valid(2, ends, types));
}
