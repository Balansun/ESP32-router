#include <gtest/gtest.h>

#include "balansun_history_limits.h"
#include "balansun_pwr_hist_limits.h"

TEST(BalansunHistoryLimits, RetainedWithinEepromRing) {
  EXPECT_GT(kBalansunHistoryDaysRetained, 0);
  EXPECT_LE(kBalansunHistoryDaysRetained, 92);
}

TEST(BalansunPwrHistLimits, RingSizesPositive) {
  EXPECT_GT(kBalansunPwrHist5mnSlots, 0);
  EXPECT_GT(kBalansunPwrHist2sSlots, 0);
  EXPECT_EQ(kBalansunPwrHist5mnWindowHours * 60, kBalansunPwrHist5mnSlots * 5);
}

TEST(BalansunPwrHistLimits, DefaultWindowMatchesRingDepth) {
#if BALANSUN_PWR_HIST_5MN_SLOTS <= 288
  EXPECT_STREQ(kBalansunPwrHistDefaultWindow, "24h");
#else
  EXPECT_STREQ(kBalansunPwrHistDefaultWindow, "48h");
#endif
}

TEST(BalansunPwrHistLimits, HistoryTemperatureSampleKeepsOneDecimal) {
  EXPECT_FLOAT_EQ(balansun_history_temperature_sample(19.04f), 19.0f);
  EXPECT_FLOAT_EQ(balansun_history_temperature_sample(19.06f), 19.1f);
  EXPECT_FLOAT_EQ(balansun_history_temperature_sample(19.1f), 19.1f);
  EXPECT_FLOAT_EQ(balansun_history_temperature_sample(-127.f), 0.f);
}
