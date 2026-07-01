#include <gtest/gtest.h>

#include "balansun_output_suspend_logic.h"
#include "balansun_product_profile.h"

TEST(BalansunOutputSuspend, SafetyLockoutBeatsVacation) {
  BalansunOutputSuspendInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  in.safety_lockout_active = true;
  in.vacation_active = true;
  in.source_stale = true;
  const auto r = balansun_output_suspend_eval(in);
  EXPECT_TRUE(r.active);
  EXPECT_EQ(r.reason, BalansunOutputSuspendReason::SafetyLockout);
  EXPECT_STREQ(balansun_output_suspend_reason_wire(r.reason), "safety_lockout");
}

TEST(BalansunOutputSuspend, VacationBeatsStale) {
  BalansunOutputSuspendInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  in.vacation_active = true;
  in.triac_off_when_source_stale = true;
  in.source_stale = true;
  const auto r = balansun_output_suspend_eval(in);
  EXPECT_TRUE(r.active);
  EXPECT_EQ(r.reason, BalansunOutputSuspendReason::Vacation);
}

TEST(BalansunOutputSuspend, StaleWhenEnabled) {
  BalansunOutputSuspendInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  in.triac_off_when_source_stale = true;
  in.source_stale = true;
  const auto r = balansun_output_suspend_eval(in);
  EXPECT_TRUE(r.active);
  EXPECT_EQ(r.reason, BalansunOutputSuspendReason::SourceStale);
}

TEST(BalansunOutputSuspend, MeterProfileNeverSuspends) {
  BalansunOutputSuspendInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_METER_ONLY);
  in.vacation_active = true;
  const auto r = balansun_output_suspend_eval(in);
  EXPECT_FALSE(r.active);
}

TEST(BalansunOutputSuspend, CommissioningBlocksDuringSelfTest) {
  BalansunOutputSuspendInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  in.commissioning_blocks_outputs = true;
  in.self_test_running = true;
  const auto r = balansun_output_suspend_eval(in);
  EXPECT_TRUE(r.active);
  EXPECT_EQ(r.reason, BalansunOutputSuspendReason::Commissioning);
  EXPECT_STREQ(balansun_output_suspend_reason_wire(r.reason), "commissioning");
  EXPECT_STREQ(balansun_output_suspend_reason_wire(BalansunOutputSuspendReason::None), "none");
  EXPECT_STREQ(balansun_output_suspend_reason_wire(BalansunOutputSuspendReason::SourceStale), "source_stale");
  EXPECT_STREQ(balansun_output_suspend_reason_wire(BalansunOutputSuspendReason::Vacation), "vacation");
}

TEST(BalansunOutputSuspend, BootSelfTestDoesNotSuspend) {
  BalansunOutputSuspendInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  in.self_test_blocks_outputs = true;
  const auto r = balansun_output_suspend_eval(in);
  EXPECT_FALSE(r.active);
}

TEST(BalansunOutputSuspend, InactiveWhenNoTriggers) {
  BalansunOutputSuspendInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  const auto r = balansun_output_suspend_eval(in);
  EXPECT_FALSE(r.active);
  EXPECT_EQ(r.reason, BalansunOutputSuspendReason::None);
}

TEST(BalansunOutputSuspend, BranchGuards) {
  BalansunProductCaps caps{};
  caps.mask = static_cast<uint32_t>(BalansunCap::SurplusRegulation);
  BalansunOutputSuspendInput in{};
  in.caps = caps;
  in.vacation_active = true;
  EXPECT_FALSE(balansun_output_suspend_eval(in).active);
  in.vacation_active = false;
  in.source_stale = true;
  in.triac_off_when_source_stale = false;
  EXPECT_FALSE(balansun_output_suspend_eval(in).active);
  in.triac_off_when_source_stale = true;
  EXPECT_TRUE(balansun_output_suspend_eval(in).active);
  in.source_stale = false;
  in.commissioning_blocks_outputs = true;
  in.self_test_running = false;
  EXPECT_FALSE(balansun_output_suspend_eval(in).active);
}
