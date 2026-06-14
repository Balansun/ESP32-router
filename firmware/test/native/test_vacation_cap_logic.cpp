#include <gtest/gtest.h>

#include "balansun_action_cap_logic.h"
#include "balansun_site_cap_logic.h"
#include "balansun_vacation_logic.h"

TEST(VacationLogic, ActiveUntilEnd) {
  EXPECT_TRUE(balansun_vacation_logic_active(true, 2000, 1000));
  EXPECT_FALSE(balansun_vacation_logic_active(true, 2000, 2000));
  EXPECT_FALSE(balansun_vacation_logic_tick_enabled(true, 2000, 2000));
}

TEST(VacationLogic, TickDisabledWhenFeatureOff) {
  EXPECT_FALSE(balansun_vacation_logic_tick_enabled(false, 2000, 1000));
}

TEST(VacationLogic, DisabledAndOpenEnded) {
  EXPECT_FALSE(balansun_vacation_logic_active(false, 2000, 1000));
  EXPECT_TRUE(balansun_vacation_logic_active(true, 0, 5000));
  EXPECT_TRUE(balansun_vacation_logic_tick_enabled(true, 0, 500));
  EXPECT_TRUE(balansun_vacation_logic_tick_enabled(true, 2000, 500));
  EXPECT_TRUE(balansun_vacation_logic_tick_enabled(true, 3000, 1500));
}

TEST(ActionCapLogic, HitAtCap) {
  ActionCapInput in{5000, 5000};
  EXPECT_TRUE(balansun_action_cap_logic_is_hit(in));
  in.routed_wh_today = 4999;
  EXPECT_FALSE(balansun_action_cap_logic_is_hit(in));
}

TEST(ActionCapLogic, ZeroCapNeverHits) {
  ActionCapInput in{0, 99999};
  EXPECT_FALSE(balansun_action_cap_logic_is_hit(in));
}

TEST(SiteCapLogic, ScalesDown) {
  EXPECT_EQ(balansun_site_cap_logic_clamp_triac_open(80, 4000, 2000), 40);
}

TEST(SiteCapLogic, NoOpWhenCapDisabled) {
  EXPECT_EQ(balansun_site_cap_logic_clamp_triac_open(80, 4000, 0), 80);
  EXPECT_EQ(balansun_site_cap_logic_clamp_triac_open(0, 4000, 2000), 0);
  EXPECT_EQ(balansun_site_cap_logic_clamp_triac_open(80, 0, 2000), 80);
  EXPECT_EQ(balansun_site_cap_logic_clamp_triac_open(400, 5000, 2000), 100);
}

TEST(SiteCapLogic, ScalesDownToCeiling) {
  EXPECT_EQ(balansun_site_cap_logic_clamp_triac_open(100, 200, 500), 100);
  EXPECT_EQ(balansun_site_cap_logic_clamp_triac_open(0, 8000, 2000), 0);
}

TEST(VacationLogic, TickEndsAfterEpoch) {
  EXPECT_FALSE(balansun_vacation_logic_tick_enabled(true, 1000, 2000));
  EXPECT_FALSE(balansun_vacation_logic_active(true, 1000, 2000));
}

TEST(VacationLogic, UnsyncedClockStaysActive) {
  EXPECT_TRUE(balansun_vacation_logic_active(true, 500, 100));
  EXPECT_TRUE(balansun_vacation_logic_tick_enabled(true, 500, 100));
}

TEST(SiteCapLogic, ScalesAndClampsHighTriac) {
  EXPECT_EQ(balansun_site_cap_logic_clamp_triac_open(500, 3000, 2000), 100);
}

TEST(SiteCapLogic, NoScaleWhenUnderCap) {
  EXPECT_EQ(balansun_site_cap_logic_clamp_triac_open(80, 1500, 2000), 80);
}
