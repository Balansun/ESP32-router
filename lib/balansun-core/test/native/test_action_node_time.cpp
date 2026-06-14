#include <gtest/gtest.h>

#include <balansun/action_node_time.h>

TEST(ActionNodeTime, WallDecihoursFromHm) {
  EXPECT_EQ(balansun_wall_decihours_from_hm(6, 0), 600);
  EXPECT_EQ(balansun_wall_decihours_from_hm(23, 59), 2359);
  EXPECT_EQ(balansun_wall_decihours_from_hm(-1, 0), 0);
  EXPECT_EQ(balansun_wall_decihours_from_hm(25, 70), 2359);
}

TEST(ActionNodeTime, InvalidEpochReturnsZero) {
  EXPECT_FALSE(balansun_time_is_valid(0));
  EXPECT_FALSE(balansun_time_is_valid(-1));
  EXPECT_EQ(balansun_wall_decihours_from_time(0, 0), 0);
}

TEST(ActionNodeTime, ValidEpochProducesDecihours) {
  struct tm parts {};
  parts.tm_year = 2026 - 1900;
  parts.tm_mon = 5;
  parts.tm_mday = 14;
  parts.tm_hour = 6;
  parts.tm_min = 30;
  parts.tm_sec = 0;
  parts.tm_isdst = 0;
#if defined(_WIN32)
  const time_t ts = _mkgmtime(&parts);
#else
  const time_t ts = timegm(&parts);
#endif
  ASSERT_TRUE(balansun_time_is_valid(ts));
  EXPECT_EQ(balansun_wall_decihours_from_time(ts, 0), 630);
}
