#include <gtest/gtest.h>

#include "balansun_product_caps_logic.h"
#include "balansun_product_profile.h"
#include "balansun_triac_sync_logic.h"

TEST(BalansunTriacSync, NotApplicableOnMeterProfile) {
  const BalansunProductCaps caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_METER_ONLY);
  EXPECT_EQ(balansun_triac_sync_state_from_zc(5, caps), BalansunTriacSyncState::NotApplicable);
  EXPECT_STREQ(balansun_triac_sync_state_wire(BalansunTriacSyncState::NotApplicable), "not_applicable");
}

TEST(BalansunTriacSync, ZcStatesOnRouter) {
  const BalansunProductCaps caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  EXPECT_EQ(balansun_triac_sync_state_from_zc(-1, caps), BalansunTriacSyncState::Fallback10ms);
  EXPECT_STREQ(balansun_triac_sync_state_wire(BalansunTriacSyncState::Fallback10ms), "fallback_10ms");
  EXPECT_EQ(balansun_triac_sync_state_from_zc(0, caps), BalansunTriacSyncState::Unsynced);
  EXPECT_STREQ(balansun_triac_sync_state_wire(BalansunTriacSyncState::Unsynced), "unsynced");
  EXPECT_EQ(balansun_triac_sync_state_from_zc(2, caps), BalansunTriacSyncState::Acquiring);
  EXPECT_STREQ(balansun_triac_sync_state_wire(BalansunTriacSyncState::Acquiring), "acquiring");
  EXPECT_EQ(balansun_triac_sync_state_from_zc(5, caps), BalansunTriacSyncState::Synced);
  EXPECT_STREQ(balansun_triac_sync_state_wire(BalansunTriacSyncState::Synced), "synced");
}
