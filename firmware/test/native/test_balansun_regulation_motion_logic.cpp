#include <gtest/gtest.h>

#include "balansun_regulation_motion_logic.h"
#include "balansun_product_profile.h"

TEST(BalansunRegulationMotion, Increasing) {
  BalansunRegulationMotionInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  in.prev_open_percent = 10;
  in.new_open_percent = 25;
  EXPECT_EQ(balansun_regulation_motion_compute(in), BalansunRegulationMotion::Increasing);
}

TEST(BalansunRegulationMotion, IdleWhenSuspended) {
  BalansunRegulationMotionInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  in.suspend_active = true;
  in.prev_open_percent = 10;
  in.new_open_percent = 50;
  EXPECT_EQ(balansun_regulation_motion_compute(in), BalansunRegulationMotion::Idle);
}

TEST(BalansunRegulationMotion, DecreasingAndLimits) {
  BalansunRegulationMotionInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  in.prev_open_percent = 40;
  in.new_open_percent = 20;
  EXPECT_EQ(balansun_regulation_motion_compute(in), BalansunRegulationMotion::Decreasing);
  EXPECT_STREQ(balansun_regulation_motion_wire(BalansunRegulationMotion::Decreasing), "decreasing");
  in.new_open_percent = 0;
  in.floor_percent = 0;
  EXPECT_EQ(balansun_regulation_motion_compute(in), BalansunRegulationMotion::AtMinimum);
  in.new_open_percent = 100;
  in.ceiling_percent = 100;
  EXPECT_EQ(balansun_regulation_motion_compute(in), BalansunRegulationMotion::AtMaximum);
  in.prev_open_percent = 30;
  in.new_open_percent = 32;
  in.deadband_percent = 5;
  EXPECT_EQ(balansun_regulation_motion_compute(in), BalansunRegulationMotion::Idle);
}

TEST(BalansunRegulationMotion, MeterProfileIdle) {
  BalansunRegulationMotionInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_METER_ONLY);
  in.prev_open_percent = 0;
  in.new_open_percent = 80;
  EXPECT_EQ(balansun_regulation_motion_compute(in), BalansunRegulationMotion::Idle);
}

TEST(BalansunRegulationMotion, ScheduleBlocked) {
  BalansunRegulationMotionInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  in.schedule_allows_surplus = false;
  in.prev_open_percent = 5;
  in.new_open_percent = 50;
  EXPECT_EQ(balansun_regulation_motion_compute(in), BalansunRegulationMotion::Idle);
  EXPECT_STREQ(balansun_regulation_motion_wire(BalansunRegulationMotion::Increasing), "increasing");
  EXPECT_STREQ(balansun_regulation_motion_wire(BalansunRegulationMotion::Idle), "idle");
  EXPECT_STREQ(balansun_regulation_motion_wire(BalansunRegulationMotion::AtMinimum), "at_minimum");
  EXPECT_STREQ(balansun_regulation_motion_wire(BalansunRegulationMotion::AtMaximum), "at_maximum");
}

TEST(BalansunRegulationMotion, FloorCeilingAndDeadbandEdges) {
  BalansunRegulationMotionInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  in.floor_percent = 10;
  in.new_open_percent = 8;
  EXPECT_EQ(balansun_regulation_motion_compute(in), BalansunRegulationMotion::AtMinimum);
  in.new_open_percent = 95;
  in.ceiling_percent = 90;
  EXPECT_EQ(balansun_regulation_motion_compute(in), BalansunRegulationMotion::AtMaximum);
  in.floor_percent = 0;
  in.ceiling_percent = 100;
  in.prev_open_percent = 20;
  in.new_open_percent = 23;
  in.deadband_percent = 3;
  EXPECT_EQ(balansun_regulation_motion_compute(in), BalansunRegulationMotion::Idle);
  in.new_open_percent = 24;
  EXPECT_EQ(balansun_regulation_motion_compute(in), BalansunRegulationMotion::Increasing);
  in.prev_open_percent = 40;
  in.new_open_percent = 37;
  EXPECT_EQ(balansun_regulation_motion_compute(in), BalansunRegulationMotion::Idle);
  in.new_open_percent = 36;
  EXPECT_EQ(balansun_regulation_motion_compute(in), BalansunRegulationMotion::Decreasing);
}
