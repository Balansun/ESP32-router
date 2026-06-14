#include <gtest/gtest.h>

#include <balansun/action_node_manual.h>

TEST(ActionNodeManual, PermanentWhileActive) {
  EXPECT_TRUE(action_node_manual_is_live(true, 0, 1000));
  EXPECT_FALSE(action_node_manual_has_expired(true, 0, 2000));
}

TEST(ActionNodeManual, TimedLiveBeforeExpiry) {
  EXPECT_TRUE(action_node_manual_is_live(true, 5000, 1000));
  EXPECT_FALSE(action_node_manual_has_expired(true, 5000, 1000));
  EXPECT_EQ(action_node_control_mode_from_manual(true), ActionNodeControlMode::Manual);
  EXPECT_STREQ(action_node_control_mode_wire(ActionNodeControlMode::Manual), "manual");
}

TEST(ActionNodeManual, TimedExpiredAfterDeadline) {
  EXPECT_FALSE(action_node_manual_is_live(true, 1000, 2000));
  EXPECT_TRUE(action_node_manual_has_expired(true, 1000, 2000));
  EXPECT_EQ(action_node_control_mode_from_manual(false), ActionNodeControlMode::Auto);
  EXPECT_STREQ(action_node_control_mode_wire(ActionNodeControlMode::Auto), "auto");
}

TEST(ActionNodeManual, MillisWraparoundExpires) {
  EXPECT_FALSE(action_node_manual_is_live(true, 50, 100));
  EXPECT_TRUE(action_node_manual_has_expired(true, 50, 100));
}

TEST(ActionNodeManual, InactiveNeverLive) {
  EXPECT_FALSE(action_node_manual_is_live(false, 5000, 1000));
  EXPECT_FALSE(action_node_manual_has_expired(false, 1000, 2000));
}
