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

TEST(BalansunDailyEnergyLogic, anchor_init_on_first_valid_read) {
  long j0 = 0;
  long floor = 0;
  EXPECT_EQ(balansun_daily_energy_wh(2190, j0, floor), 0);
  EXPECT_EQ(j0, 2190);
  EXPECT_EQ(balansun_daily_energy_wh(2195, j0, floor), 5);
}

TEST(BalansunDailyEnergyLogic, zero_cumulative_does_not_clobber_eeprom_anchor) {
  long j0 = 2190;
  long floor = 0;
  EXPECT_EQ(balansun_daily_energy_wh(0, j0, floor), 0);
  EXPECT_EQ(j0, 2190);
  EXPECT_EQ(balansun_daily_energy_wh(2193, j0, floor), 3);
}

TEST(BalansunDailyEnergyLogic, meter_rollover_reanchors) {
  long j0 = 5000;
  long floor = 0;
  EXPECT_EQ(balansun_daily_energy_wh(100, j0, floor), 0);
  EXPECT_EQ(j0, 100);
  EXPECT_EQ(balansun_daily_energy_wh(150, j0, floor), 50);
}

TEST(BalansunDailyEnergyLogic, ota_glitch_holds_j0_and_day_floor) {
  long j0 = 410;
  long floor = 801;
  EXPECT_EQ(balansun_daily_energy_wh(221, j0, floor), 801);
  EXPECT_EQ(j0, 410);
  EXPECT_EQ(floor, 801);
}

TEST(BalansunDailyEnergyLogic, routed_day_picks_max) {
  EXPECT_DOUBLE_EQ(balansun_routed_day_energy_wh(30, 0), 30);
  EXPECT_DOUBLE_EQ(balansun_routed_day_energy_wh(0, 1600), 1600);
  EXPECT_DOUBLE_EQ(balansun_routed_day_energy_wh(407, 1600), 1600);
}
