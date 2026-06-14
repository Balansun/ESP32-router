#include <gtest/gtest.h>

#include "uxi_adc_clip_logic.h"

TEST(UxiAdcClipLogic, SetsAndClears) {
  UxiAdcClipState st;
  uxi_adc_clip_logic_update(st, 100, 100);
  EXPECT_FALSE(uxi_adc_clip_logic_is_clipping(st));
  uxi_adc_clip_logic_update(st, 4095, 0);
  EXPECT_TRUE(uxi_adc_clip_logic_is_clipping(st));
  uxi_adc_clip_logic_update(st, 100, 100);
  EXPECT_TRUE(uxi_adc_clip_logic_is_clipping(st));
  uxi_adc_clip_logic_update(st, 100, 100);
  EXPECT_FALSE(uxi_adc_clip_logic_is_clipping(st));
}

TEST(UxiAdcClipLogic, VoltOnlyClip) {
  UxiAdcClipState st;
  uxi_adc_clip_logic_update(st, 4095, 100);
  EXPECT_TRUE(uxi_adc_clip_logic_is_clipping(st));
}

TEST(UxiAdcClipLogic, AmpOnlyClip) {
  UxiAdcClipState st;
  uxi_adc_clip_logic_update(st, 100, 4095);
  EXPECT_TRUE(uxi_adc_clip_logic_is_clipping(st));
}

TEST(UxiAdcClipLogic, SingleClearPollStaysClipped) {
  UxiAdcClipState st;
  uxi_adc_clip_logic_update(st, 4095, 4095);
  ASSERT_TRUE(uxi_adc_clip_logic_is_clipping(st));
  uxi_adc_clip_logic_update(st, 100, 100);
  EXPECT_TRUE(uxi_adc_clip_logic_is_clipping(st));
}

TEST(UxiAdcClipLogic, NegativeRawClampedToZero) {
  UxiAdcClipState st;
  uxi_adc_clip_logic_update(st, -50, -100);
  EXPECT_FALSE(uxi_adc_clip_logic_is_clipping(st));
}

TEST(UxiAdcClipLogic, ClippingMidBandHoldsUntilClear) {
  UxiAdcClipState st;
  uxi_adc_clip_logic_update(st, 4095, 100);
  ASSERT_TRUE(uxi_adc_clip_logic_is_clipping(st));
  uxi_adc_clip_logic_update(st, 4085, 4085);
  EXPECT_TRUE(uxi_adc_clip_logic_is_clipping(st));
  uxi_adc_clip_logic_update(st, 100, 100);
  EXPECT_TRUE(uxi_adc_clip_logic_is_clipping(st));
  uxi_adc_clip_logic_update(st, 100, 100);
  EXPECT_FALSE(uxi_adc_clip_logic_is_clipping(st));
}

TEST(UxiAdcClipLogic, ClipThresholdExactly4090) {
  UxiAdcClipState st;
  uxi_adc_clip_logic_update(st, 4090, 4090);
  EXPECT_TRUE(uxi_adc_clip_logic_is_clipping(st));
}

TEST(UxiAdcClipLogic, ClippingHighVoltOnlyBlocksClear) {
  UxiAdcClipState st;
  uxi_adc_clip_logic_update(st, 4095, 100);
  ASSERT_TRUE(uxi_adc_clip_logic_is_clipping(st));
  uxi_adc_clip_logic_update(st, 4085, 100);
  EXPECT_TRUE(uxi_adc_clip_logic_is_clipping(st));
}

TEST(UxiAdcClipLogic, ClippingHighAmpOnlyBlocksClear) {
  UxiAdcClipState st;
  uxi_adc_clip_logic_update(st, 100, 4095);
  ASSERT_TRUE(uxi_adc_clip_logic_is_clipping(st));
  uxi_adc_clip_logic_update(st, 100, 4085);
  EXPECT_TRUE(uxi_adc_clip_logic_is_clipping(st));
}

TEST(UxiAdcClipLogic, IdleMidBandDoesNotClip) {
  UxiAdcClipState st;
  uxi_adc_clip_logic_update(st, 4085, 4085);
  EXPECT_FALSE(uxi_adc_clip_logic_is_clipping(st));
}

TEST(UxiAdcClipLogic, NotClippingResetsClearStreakOnReclip) {
  UxiAdcClipState st;
  uxi_adc_clip_logic_update(st, 4095, 100);
  ASSERT_TRUE(uxi_adc_clip_logic_is_clipping(st));
  uxi_adc_clip_logic_update(st, 100, 100);
  ASSERT_TRUE(uxi_adc_clip_logic_is_clipping(st));
  uxi_adc_clip_logic_update(st, 4095, 100);
  EXPECT_TRUE(uxi_adc_clip_logic_is_clipping(st));
  uxi_adc_clip_logic_update(st, 100, 100);
  EXPECT_TRUE(uxi_adc_clip_logic_is_clipping(st));
  uxi_adc_clip_logic_update(st, 100, 100);
  EXPECT_FALSE(uxi_adc_clip_logic_is_clipping(st));
  uxi_adc_clip_logic_update(st, 200, 300);
  EXPECT_FALSE(uxi_adc_clip_logic_is_clipping(st));
}
