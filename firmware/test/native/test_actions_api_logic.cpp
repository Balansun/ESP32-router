#include <gtest/gtest.h>

#include "actions_api_logic.h"
#include "actions_logic.h"
#include "balansun_board.h"

TEST(ActionsApiLogic, IndexBounds) {
  EXPECT_TRUE(actions_api_logic_is_valid_action_index(0, kMaxRoutingActions));
  EXPECT_FALSE(actions_api_logic_is_valid_action_index(kMaxRoutingActions, kMaxRoutingActions));
}

TEST(ActionsApiLogic, OverrideValidation) {
  auto v = actions_api_logic_validate_override(0, kMaxRoutingActions, "auto", 0, 20.0f, 70);
  EXPECT_TRUE(v.ok);
  EXPECT_EQ(v.override_state, kActionOverrideAuto);

  v = actions_api_logic_validate_override(1, kMaxRoutingActions, "triac_fixed", 50, 20.0f, 70);
  EXPECT_FALSE(v.ok);

  v = actions_api_logic_validate_override(0, kMaxRoutingActions, "triac_fixed", 150, 20.0f, 70);
  EXPECT_FALSE(v.ok);

  v = actions_api_logic_validate_override(0, kMaxRoutingActions, "triac_fixed", 40, 20.0f, 70);
  EXPECT_TRUE(v.ok);
  EXPECT_EQ(v.override_state, kActionOverrideTriacFixed);
}

TEST(ActionsApiLogic, TriacFixedBlockedAboveTempCap) {
  auto v = actions_api_logic_validate_override(0, kMaxRoutingActions, "triac_fixed", 100, 72.0f, 70);
  EXPECT_FALSE(v.ok);
  EXPECT_NE(v.error.find("temperature"), std::string::npos);

  v = actions_api_logic_validate_override(0, kMaxRoutingActions, "triac_fixed", 100, 72.0f, 0);
  EXPECT_TRUE(v.ok);

  v = actions_api_logic_validate_override(0, kMaxRoutingActions, "triac_fixed", 99, 80.0f, 70);
  EXPECT_TRUE(v.ok);
}

TEST(ActionsApiLogic, OverrideStatesOnOff) {
  auto v = actions_api_logic_validate_override(0, kMaxRoutingActions, "on", 0, 20.0f, 70);
  EXPECT_TRUE(v.ok);
  EXPECT_EQ(v.override_state, kActionOverrideOn);
  v = actions_api_logic_validate_override(0, kMaxRoutingActions, "off", 0, 20.0f, 70);
  EXPECT_TRUE(v.ok);
  EXPECT_EQ(v.override_state, kActionOverrideOff);
}

TEST(ActionsApiLogic, TriacFixedRejectsNegativePercent) {
  auto v = actions_api_logic_validate_override(0, kMaxRoutingActions, "triac_fixed", -1, 20.0f, 70);
  EXPECT_FALSE(v.ok);
}

TEST(ActionsApiLogic, OverrideStatesCaseInsensitive) {
  auto v = actions_api_logic_validate_override(0, kMaxRoutingActions, "AUTO", 0, 20.0f, 70);
  EXPECT_TRUE(v.ok);
  EXPECT_EQ(v.override_state, kActionOverrideAuto);
}

TEST(ActionsApiLogic, RejectsBadIndexAndState) {
  auto v = actions_api_logic_validate_override(-1, kMaxRoutingActions, "auto", 0, 20.0f, 70);
  EXPECT_FALSE(v.ok);
  v = actions_api_logic_validate_override(0, kMaxRoutingActions, nullptr, 0, 20.0f, 70);
  EXPECT_FALSE(v.ok);
  v = actions_api_logic_validate_override(0, kMaxRoutingActions, "bogus", 0, 20.0f, 70);
  EXPECT_FALSE(v.ok);
}
