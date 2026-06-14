#include <gtest/gtest.h>

#include "balansun_product_caps_logic.h"
#include "balansun_product_profile.h"

static void expect_router_caps(const BalansunProductCaps &caps) {
  EXPECT_TRUE(balansun_product_caps_has(caps, BalansunCap::SurplusRegulation));
  EXPECT_TRUE(balansun_product_caps_has(caps, BalansunCap::TriacDimming));
}

static void expect_meter_gateway_caps(const BalansunProductCaps &caps) {
  EXPECT_FALSE(balansun_product_caps_has(caps, BalansunCap::SurplusRegulation));
  EXPECT_FALSE(balansun_product_caps_has(caps, BalansunCap::TriacDimming));
}

TEST(BalansunProductCapsInvariant, RoleMasks) {
  expect_meter_gateway_caps(balansun_product_caps_for_role(BALANSUN_ROLE_METER_GATEWAY));
  expect_router_caps(balansun_product_caps_for_role(BALANSUN_ROLE_METER_ROUTER));
  expect_router_caps(balansun_product_caps_for_role(BALANSUN_ROLE_FULL_ROUTER));
}

TEST(BalansunProductCapsInvariant, ProfileWires) {
  expect_meter_gateway_caps(balansun_product_caps_for_profile(BALANSUN_PRODUCT_JSY_MK194_METER));
  expect_meter_gateway_caps(balansun_product_caps_for_profile(BALANSUN_PRODUCT_METER_ONLY));
  expect_router_caps(balansun_product_caps_for_profile(BALANSUN_PRODUCT_JSY_MK194_ROUTER));
  expect_router_caps(balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER));
}

TEST(BalansunProductCapsInvariant, TriacSubsetOfSurplusRegulation) {
  const int roles[] = {BALANSUN_ROLE_METER_GATEWAY, BALANSUN_ROLE_METER_ROUTER, BALANSUN_ROLE_FULL_ROUTER};
  for (const int role : roles) {
    const BalansunProductCaps caps = balansun_product_caps_for_role(role);
    if (balansun_product_caps_has(caps, BalansunCap::TriacDimming)) {
      EXPECT_TRUE(balansun_product_caps_has(caps, BalansunCap::SurplusRegulation));
    }
  }
}
