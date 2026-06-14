#include <gtest/gtest.h>

#include "balansun_hw_profile_logic.h"

TEST(HwProfile, WroomConstrained) {
  const BalansunHwCaps caps =
      balansun_hw_caps_effective(BalansunHwCeiling::Constrained, false, 0, 200000);
  EXPECT_EQ(caps.hist_abs_max_points, 200);
  EXPECT_EQ(caps.config_export_json_cap, 20480u);
  EXPECT_EQ(caps.put_body_max, caps.config_export_json_cap);
  EXPECT_EQ(caps.backup_part_json_cap, 12288u);
  EXPECT_EQ(caps.pmqtt_bindings_apply_cap, 4096u);
  EXPECT_FALSE(caps.full_config_export);
}

TEST(HwProfile, WroverExtended) {
  const BalansunHwCaps caps = balansun_hw_caps_effective(BalansunHwCeiling::Extended, true, 8u * 1024u * 1024u, 200000);
  EXPECT_EQ(caps.hist_abs_max_points, 600);
  EXPECT_EQ(caps.config_export_json_cap, 49152u);
  EXPECT_EQ(caps.put_body_max, 49152u);
  EXPECT_TRUE(caps.full_config_export);
}

TEST(HwProfile, S3NoPsramStandard) {
  const BalansunHwCaps caps = balansun_hw_caps_effective(BalansunHwCeiling::Extended, false, 0, 200000);
  EXPECT_EQ(caps.hist_abs_max_points, 400);
  EXPECT_TRUE(caps.full_config_export);
}

TEST(HwProfile, LowHeapDemotes) {
  BalansunHwCaps caps = balansun_hw_caps_for_tier(BalansunHwTier::Extended);
  const BalansunHwCaps demoted = balansun_hw_caps_demote_if_low_heap(caps, 30000);
  EXPECT_EQ(demoted.hist_abs_max_points, 200);
}

TEST(HwProfile, WroomCeilingBlocksExtendedEvenWithPsram) {
  const BalansunHwCaps caps =
      balansun_hw_caps_effective(BalansunHwCeiling::Constrained, true, 8u * 1024u * 1024u, 200000);
  EXPECT_EQ(caps.hist_abs_max_points, 200);
  EXPECT_FALSE(caps.full_config_export);
}

TEST(HwProfile, TierNames) {
  EXPECT_STREQ(balansun_hw_tier_name(BalansunHwTier::Constrained), "constrained");
  EXPECT_STREQ(balansun_hw_tier_name(BalansunHwTier::Standard), "standard");
  EXPECT_STREQ(balansun_hw_tier_name(BalansunHwTier::Extended), "extended");
}

TEST(HwProfile, StandardCapsTable) {
  const BalansunHwCaps caps = balansun_hw_caps_for_tier(BalansunHwTier::Standard);
  EXPECT_EQ(caps.hist_abs_max_points, 400);
  EXPECT_EQ(caps.config_export_json_cap, 32768u);
  EXPECT_EQ(caps.put_body_max, 32768u);
  EXPECT_EQ(caps.hist_power_json_cap, 20480u);
  EXPECT_TRUE(caps.full_config_export);
}

TEST(HwProfile, ExtendedCapsTable) {
  const BalansunHwCaps caps = balansun_hw_caps_for_tier(BalansunHwTier::Extended);
  EXPECT_EQ(caps.hist_power_json_cap, 32768u);
  EXPECT_EQ(caps.history_daily_import_max, 32768u);
}

TEST(HwProfile, SmallPsramOnExtendedCeilingIsStandard) {
  const BalansunHwTier tier =
      balansun_hw_tier_from_resources(BalansunHwCeiling::Extended, true, 1024u * 1024u, 200000);
  EXPECT_EQ(tier, BalansunHwTier::Standard);
}

TEST(HwProfile, LowHeapNoDemoteWhenHeapOk) {
  const BalansunHwCaps caps = balansun_hw_caps_for_tier(BalansunHwTier::Extended);
  const BalansunHwCaps same = balansun_hw_caps_demote_if_low_heap(caps, 100000);
  EXPECT_EQ(same.hist_abs_max_points, 600);
}

TEST(HwProfile, LowHeapNoDemoteWhenAlreadyConstrained) {
  const BalansunHwCaps caps = balansun_hw_caps_for_tier(BalansunHwTier::Constrained);
  const BalansunHwCaps same = balansun_hw_caps_demote_if_low_heap(caps, 1000);
  EXPECT_EQ(same.hist_abs_max_points, 200);
}
