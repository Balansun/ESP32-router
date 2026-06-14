#include <gtest/gtest.h>

#include "balansun_config_daily_cap_logic.h"

TEST(ConfigDailyCapLogic, PartialArrayUpdatesFirstSlots) {
  uint32_t caps[3] = {100, 200, 300};
  const uint32_t patch[] = {5000, 8000};
  balansun_config_daily_cap_apply(caps, 3, patch, 2);
  EXPECT_EQ(caps[0], 5000u);
  EXPECT_EQ(caps[1], 8000u);
  EXPECT_EQ(caps[2], 300u);
}

TEST(ConfigDailyCapLogic, EmptyOrNullIsNoOp) {
  uint32_t caps[2] = {42, 43};
  balansun_config_daily_cap_apply(caps, 2, nullptr, 1);
  balansun_config_daily_cap_apply(caps, 2, caps, 0);
  balansun_config_daily_cap_apply(nullptr, 2, caps, 1);
  balansun_config_daily_cap_apply(caps, 0, caps, 1);
  EXPECT_EQ(caps[0], 42u);
  EXPECT_EQ(caps[1], 43u);
}

TEST(ConfigDailyCapLogic, ClampsToNbActions) {
  uint32_t caps[2] = {0, 0};
  const uint32_t patch[] = {1, 2, 3, 4};
  balansun_config_daily_cap_apply(caps, 2, patch, 4);
  EXPECT_EQ(caps[0], 1u);
  EXPECT_EQ(caps[1], 2u);
}
