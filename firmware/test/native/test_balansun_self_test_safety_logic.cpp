#include <gtest/gtest.h>

#include "balansun_product_profile.h"
#include "balansun_self_test_safety_logic.h"

namespace {

BalansunSelfTestSafetyInput router_input(SelfTestPersisted &st, bool stale_guard, bool running = false) {
  BalansunSelfTestSafetyInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  in.self_test = &st;
  in.triac_off_when_source_stale = stale_guard;
  in.self_test_running = running;
  return in;
}

}  // namespace

TEST(BalansunSelfTestSafety, MeterProfileNeverLockout) {
  SelfTestPersisted st{};
  st.run_epoch = 100;
  st.zc_ok = false;
  BalansunSelfTestSafetyInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_METER_ONLY);
  in.self_test = &st;
  const auto r = balansun_self_test_safety_eval(in);
  EXPECT_FALSE(r.lockout_active);
}

TEST(BalansunSelfTestSafety, SkippedOrNeverRunNoLockout) {
  SelfTestPersisted st{};
  st.skipped = true;
  st.zc_ok = false;
  auto r = balansun_self_test_safety_eval(router_input(st, true));
  EXPECT_FALSE(r.lockout_active);

  st.skipped = false;
  st.run_epoch = 0;
  r = balansun_self_test_safety_eval(router_input(st, true));
  EXPECT_FALSE(r.lockout_active);
}

TEST(BalansunSelfTestSafety, ZcFailLockout) {
  SelfTestPersisted st{};
  st.run_epoch = 1;
  st.zc_ok = false;
  st.triac_ok = true;
  st.source_ok = true;
  const auto r = balansun_self_test_safety_eval(router_input(st, false));
  EXPECT_TRUE(r.lockout_active);
  EXPECT_TRUE(r.zc_critical);
  EXPECT_FALSE(r.triac_critical);
}

TEST(BalansunSelfTestSafety, TriacFailLockout) {
  SelfTestPersisted st{};
  st.run_epoch = 1;
  st.zc_ok = true;
  st.triac_ok = false;
  const auto r = balansun_self_test_safety_eval(router_input(st, false));
  EXPECT_TRUE(r.lockout_active);
  EXPECT_TRUE(r.triac_critical);
}

TEST(BalansunSelfTestSafety, SourceFailOnlyWhenStaleGuard) {
  SelfTestPersisted st{};
  st.run_epoch = 1;
  st.zc_ok = true;
  st.triac_ok = true;
  st.source_ok = false;

  auto r = balansun_self_test_safety_eval(router_input(st, false));
  EXPECT_FALSE(r.lockout_active);
  EXPECT_FALSE(r.source_critical);

  r = balansun_self_test_safety_eval(router_input(st, true));
  EXPECT_TRUE(r.lockout_active);
  EXPECT_TRUE(r.source_critical);
}

TEST(BalansunSelfTestSafety, PassClearsLockout) {
  SelfTestPersisted st{};
  st.run_epoch = 2;
  st.zc_ok = true;
  st.triac_ok = true;
  st.source_ok = true;
  const auto r = balansun_self_test_safety_eval(router_input(st, true));
  EXPECT_FALSE(r.lockout_active);
}

TEST(BalansunSelfTestSafety, RunningDoesNotLatchLockout) {
  SelfTestPersisted st{};
  st.run_epoch = 0;
  st.zc_ok = false;
  const auto r = balansun_self_test_safety_eval(router_input(st, true, true));
  EXPECT_FALSE(r.lockout_active);
  EXPECT_TRUE(r.zc_critical);
}

TEST(BalansunSelfTestSafety, LockoutReasons) {
  BalansunSelfTestSafetyResult r{};
  r.zc_critical = true;
  r.source_critical = true;
  const char *reasons[4];
  size_t count = 0;
  balansun_self_test_safety_lockout_reasons(r, reasons, 4, &count);
  EXPECT_EQ(count, 2u);
  EXPECT_STREQ(reasons[0], "zc_failed");
  EXPECT_STREQ(reasons[1], "source_failed");
}

TEST(BalansunSelfTestSafety, CompletedRunNullPtr) {
  EXPECT_FALSE(balansun_self_test_safety_completed_run(nullptr));
  SelfTestPersisted skipped{};
  skipped.skipped = true;
  skipped.run_epoch = 1;
  EXPECT_FALSE(balansun_self_test_safety_completed_run(&skipped));
}

TEST(BalansunSelfTestSafety, NullSelfTestNoEval) {
  BalansunSelfTestSafetyInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  in.self_test = nullptr;
  const auto r = balansun_self_test_safety_eval(in);
  EXPECT_FALSE(r.lockout_active);
  EXPECT_FALSE(balansun_self_test_safety_has_critical_failure(r));
}

TEST(BalansunSelfTestSafety, RunningTriacAndSourceFlags) {
  SelfTestPersisted st{};
  st.zc_ok = true;
  st.triac_ok = false;
  st.source_ok = false;
  const auto r = balansun_self_test_safety_eval(router_input(st, true, true));
  EXPECT_FALSE(r.lockout_active);
  EXPECT_FALSE(r.zc_critical);
  EXPECT_TRUE(r.triac_critical);
  EXPECT_TRUE(r.source_critical);
}

TEST(BalansunSelfTestSafety, RunningAllChecksPass) {
  SelfTestPersisted st{};
  st.zc_ok = true;
  st.triac_ok = true;
  st.source_ok = true;
  const auto r = balansun_self_test_safety_eval(router_input(st, true, true));
  EXPECT_FALSE(r.lockout_active);
  EXPECT_FALSE(balansun_self_test_safety_has_critical_failure(r));
}

TEST(BalansunSelfTestSafety, RunningSourceFailWithoutStaleGuard) {
  SelfTestPersisted st{};
  st.zc_ok = true;
  st.triac_ok = true;
  st.source_ok = false;
  const auto r = balansun_self_test_safety_eval(router_input(st, false, true));
  EXPECT_FALSE(r.source_critical);
  EXPECT_FALSE(r.lockout_active);
}

TEST(BalansunSelfTestSafety, HelpersAndSeverityWires) {
  EXPECT_STREQ(balansun_self_test_safety_lockout_wire(), "safety_lockout");

  BalansunSelfTestSafetyResult zc_only{};
  zc_only.zc_critical = true;
  EXPECT_TRUE(balansun_self_test_safety_has_critical_failure(zc_only));

  BalansunSelfTestSafetyResult source_only{};
  source_only.source_critical = true;
  EXPECT_TRUE(balansun_self_test_safety_has_critical_failure(source_only));

  BalansunSelfTestSafetyResult triac_only{};
  triac_only.triac_critical = true;
  EXPECT_TRUE(balansun_self_test_safety_has_critical_failure(triac_only));

  EXPECT_STREQ(balansun_self_test_severity_wire(true, false), "ok");
  EXPECT_STREQ(balansun_self_test_severity_wire(false, true), "critical");
  EXPECT_STREQ(balansun_self_test_severity_wire(false, false), "warning");
}

TEST(BalansunSelfTestSafety, LockoutReasonsEdgeCases) {
  size_t count = 99;
  balansun_self_test_safety_lockout_reasons({}, nullptr, 0, &count);
  EXPECT_EQ(count, 0u);

  BalansunSelfTestSafetyResult all{};
  all.zc_critical = true;
  all.triac_critical = true;
  all.source_critical = true;
  const char *reasons[3];
  count = 0;
  balansun_self_test_safety_lockout_reasons(all, reasons, 3, &count);
  EXPECT_EQ(count, 3u);
  EXPECT_STREQ(reasons[0], "zc_failed");
  EXPECT_STREQ(reasons[1], "triac_failed");
  EXPECT_STREQ(reasons[2], "source_failed");

  BalansunSelfTestSafetyResult triac{};
  triac.triac_critical = true;
  const char *one[1];
  count = 0;
  balansun_self_test_safety_lockout_reasons(triac, one, 1, &count);
  EXPECT_EQ(count, 1u);
  EXPECT_STREQ(one[0], "triac_failed");

  count = 0;
  balansun_self_test_safety_lockout_reasons(all, one, 1, &count);
  EXPECT_EQ(count, 1u);
  EXPECT_STREQ(one[0], "zc_failed");

  const char *buf[2];
  balansun_self_test_safety_lockout_reasons(all, buf, 2, nullptr);
  count = 99;
  balansun_self_test_safety_lockout_reasons(all, buf, 0, &count);
  EXPECT_EQ(count, 0u);
  balansun_self_test_safety_lockout_reasons(all, nullptr, 0, nullptr);

  BalansunSelfTestSafetyResult none{};
  count = 0;
  balansun_self_test_safety_lockout_reasons(none, one, 1, &count);
  EXPECT_EQ(count, 0u);

  BalansunSelfTestSafetyResult triac_only{};
  triac_only.triac_critical = true;
  count = 0;
  balansun_self_test_safety_lockout_reasons(triac_only, one, 1, &count);
  EXPECT_EQ(count, 1u);
  EXPECT_STREQ(one[0], "triac_failed");
}
