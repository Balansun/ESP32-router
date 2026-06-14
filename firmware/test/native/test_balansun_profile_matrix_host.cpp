#include <gtest/gtest.h>

#include <cstring>

#include "balansun_product_caps_logic.h"
#include "balansun_product_profile.h"

#include "profile_matrix_cases.inc"

static int role_from_wire(const char *role) {
  if (strcmp(role, "meter_gateway") == 0) return BALANSUN_ROLE_METER_GATEWAY;
  if (strcmp(role, "meter_router") == 0) return BALANSUN_ROLE_METER_ROUTER;
  return BALANSUN_ROLE_FULL_ROUTER;
}

TEST(ProfileMatrixHost, RoleCapsMatchGeneratedMatrix) {
  for (size_t i = 0; i < kProfileMatrixCaseCount; i++) {
    const ProfileMatrixCase &c = kProfileMatrixCases[i];
    const BalansunProductCaps caps = balansun_product_caps_for_role(role_from_wire(c.role));
    EXPECT_EQ(balansun_product_caps_has(caps, BalansunCap::SurplusRegulation), c.surplus_regulation)
        << c.product_profile;
    EXPECT_EQ(balansun_product_caps_has(caps, BalansunCap::TriacDimming), c.triac) << c.product_profile;
    EXPECT_EQ(balansun_product_caps_has(caps, BalansunCap::MultiAction), c.multi_action)
        << c.product_profile;
    EXPECT_EQ(balansun_product_caps_has(caps, BalansunCap::SourceTestInject), c.source_test_inject)
        << c.product_profile;
  }
}
