#include <gtest/gtest.h>

#include <balansun/product_caps.h>
#include <balansun/product_role.h>

TEST(BalansunProductCaps, FullRouterHasRegulation) {
  const BalansunProductCaps caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  EXPECT_STREQ(caps.profile_wire, "full_router");
  EXPECT_TRUE(balansun_product_caps_has(caps, BalansunCap::SurplusRegulation));
  EXPECT_TRUE(balansun_product_caps_has(caps, BalansunCap::MultiAction));
}

TEST(BalansunProductCaps, JsyRouterHasTriacNotMultiAction) {
  const BalansunProductCaps caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_JSY_MK194_ROUTER);
  EXPECT_STREQ(caps.profile_wire, "jsy_mk194_router");
  EXPECT_TRUE(balansun_product_caps_has(caps, BalansunCap::SurplusRegulation));
  EXPECT_TRUE(balansun_product_caps_has(caps, BalansunCap::TriacDimming));
  EXPECT_FALSE(balansun_product_caps_has(caps, BalansunCap::MultiAction));
}

TEST(BalansunProductCaps, MeterOnlyNoRegulation) {
  const BalansunProductCaps caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_METER_ONLY);
  EXPECT_FALSE(balansun_product_caps_has(caps, BalansunCap::SurplusRegulation));
  EXPECT_FALSE(balansun_product_caps_has(caps, BalansunCap::TriacDimming));
}

TEST(BalansunProductCaps, JsyMeterVariant) {
  const BalansunProductCaps caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_JSY_MK194_METER);
  EXPECT_STREQ(caps.profile_wire, "jsy_mk194_meter");
  EXPECT_FALSE(balansun_product_caps_has(caps, BalansunCap::SurplusRegulation));
  EXPECT_TRUE(balansun_product_caps_has(caps, BalansunCap::VacationMode));
}

TEST(BalansunProductCaps, WireHelpers) {
  EXPECT_STREQ(balansun_product_profile_wire(BALANSUN_PRODUCT_JSY_MK194_ROUTER), "jsy_mk194_router");
  EXPECT_STREQ(balansun_product_profile_wire(BALANSUN_PRODUCT_JSY_MK194_METER), "jsy_mk194_meter");
  EXPECT_STREQ(balansun_product_profile_wire(BALANSUN_PRODUCT_METER_ONLY), "meter_only");
  EXPECT_STREQ(balansun_product_profile_wire(999), "full_router");
  EXPECT_STREQ(balansun_product_cap_wire(BalansunCap::SourceTestInject), "source_test_inject");
  EXPECT_STREQ(balansun_product_cap_wire(static_cast<BalansunCap>(999)), "unknown");
}
