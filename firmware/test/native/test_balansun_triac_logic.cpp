#include <gtest/gtest.h>

#include "balansun_triac_logic.h"

TEST(BalansunTriacLogic, ThresholdIncreasesWithOpenPercent) {
  const int max_ticks = 100;
  const int t0 = triac_delay_threshold_ticks(0, max_ticks);
  const int t50 = triac_delay_threshold_ticks(50, max_ticks);
  const int t99 = triac_delay_threshold_ticks(99, max_ticks);
  const int tOff = triac_delay_threshold_ticks(100, max_ticks);
  EXPECT_EQ(t0, 0);
  EXPECT_LT(t0, t50);
  EXPECT_LT(t50, t99);
  EXPECT_GT(tOff, max_ticks);
}

TEST(BalansunTriacLogic, MaxTicksFromHalfPeriod) {
  const uint8_t ticks = triac_max_delay_ticks_from_half_period_us(10000);
  EXPECT_GT(ticks, 0u);
  EXPECT_LE(ticks, 100u);
}
