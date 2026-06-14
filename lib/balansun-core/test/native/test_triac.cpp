#include <gtest/gtest.h>

#include <balansun/triac.h>

TEST(TriacLogic, DelayThresholdTicks) {
  const int max_ticks = 98;
  EXPECT_EQ(balansun_triac_delay_threshold_ticks(0, max_ticks), 0);
  EXPECT_EQ(balansun_triac_delay_threshold_ticks(50, max_ticks), 49);
  EXPECT_EQ(balansun_triac_delay_threshold_ticks(100, max_ticks), 99);
}

TEST(TriacLogic, MaxDelayFromHalfPeriod) {
  EXPECT_GE(balansun_triac_max_delay_ticks_from_half_period_us(10000), 1u);
}
