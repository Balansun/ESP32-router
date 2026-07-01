#include <gtest/gtest.h>

#include "balansun_daily_energy_logic.h"

TEST(BalansunDailyEnergyLogic, yesterday_logical_index) {
  EXPECT_EQ(balansun_daily_energy_yesterday_logical_index(0), -1);
  EXPECT_EQ(balansun_daily_energy_yesterday_logical_index(1), -1);
  EXPECT_EQ(balansun_daily_energy_yesterday_logical_index(7), 5);
  EXPECT_EQ(balansun_daily_energy_yesterday_logical_index(30), 28);
}

TEST(BalansunDailyEnergyLogic, sanitize_clamps_negative) {
  long a = -1;
  long b = 10;
  long c = -3;
  long d = 0;
  balansun_daily_energy_sanitize_metrics(a, b, c, d);
  EXPECT_EQ(a, 0);
  EXPECT_EQ(b, 10);
  EXPECT_EQ(c, 0);
  EXPECT_EQ(d, 0);
}
