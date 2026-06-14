#include <gtest/gtest.h>

#include "balansun_triac_calibration_logic.h"

TEST(TriacCalibration, IdentityWhenDisabled) {
  TriacCalibrationTable cal;
  cal.enabled = false;
  EXPECT_EQ(balansun_triac_calibration_apply_open_percent(cal, 50), 50);
}

TEST(TriacCalibration, AdjustsWhenEnabled) {
  TriacCalibrationTable cal;
  cal.enabled = true;
  cal.points[0] = {50, 1000};
  cal.points[1] = {80, 2000};
  cal.points[2] = {100, 2500};
  const int out = balansun_triac_calibration_apply_open_percent(cal, 80);
  EXPECT_GE(out, 0);
  EXPECT_LE(out, 100);
}

TEST(TriacCalibration, ZeroDutyTableReturnsInput) {
  TriacCalibrationTable cal;
  cal.enabled = true;
  EXPECT_EQ(balansun_triac_calibration_apply_open_percent(cal, 42), 42);
}

TEST(TriacCalibration, LowSegmentAndZeroTarget) {
  TriacCalibrationTable cal;
  cal.enabled = true;
  cal.points[0] = {10, 0};
  cal.points[1] = {50, 1000};
  cal.points[2] = {100, 2000};
  const int low = balansun_triac_calibration_apply_open_percent(cal, 20);
  EXPECT_GE(low, 0);
  EXPECT_LE(low, 100);
  cal.points[1].measured_w = 0;
  EXPECT_EQ(balansun_triac_calibration_apply_open_percent(cal, 50), 50);
  const int high = balansun_triac_calibration_apply_open_percent(cal, 90);
  EXPECT_GE(high, 0);
  EXPECT_LE(high, 100);
}

TEST(TriacCalibration, HighSegmentAndClampInput) {
  TriacCalibrationTable cal;
  cal.enabled = true;
  cal.points[0] = {0, 0};
  cal.points[1] = {30, 500};
  cal.points[2] = {100, 3000};
  const int mid = balansun_triac_calibration_apply_open_percent(cal, 60);
  EXPECT_GE(mid, 0);
  EXPECT_LE(mid, 100);
  EXPECT_EQ(balansun_triac_calibration_apply_open_percent(cal, -10), 0);
  EXPECT_GE(balansun_triac_calibration_apply_open_percent(cal, 150), 0);
  EXPECT_LE(balansun_triac_calibration_apply_open_percent(cal, 150), 100);
}

TEST(TriacCalibration, EqualDutyLerp) {
  TriacCalibrationTable cal;
  cal.enabled = true;
  cal.points[0] = {50, 1000};
  cal.points[1] = {50, 1000};
  cal.points[2] = {100, 2000};
  EXPECT_EQ(balansun_triac_calibration_apply_open_percent(cal, 50), 50);
}

TEST(TriacCalibration, PartialZeroDutyTable) {
  TriacCalibrationTable cal;
  cal.enabled = true;
  cal.points[0] = {0, 0};
  cal.points[1] = {40, 800};
  cal.points[2] = {100, 2000};
  const int out = balansun_triac_calibration_apply_open_percent(cal, 60);
  EXPECT_GE(out, 0);
  EXPECT_LE(out, 100);
}

TEST(TriacCalibration, OnlyHighSegmentNonZeroDuty) {
  TriacCalibrationTable cal;
  cal.enabled = true;
  cal.points[0] = {0, 0};
  cal.points[1] = {0, 0};
  cal.points[2] = {50, 1200};
  const int out = balansun_triac_calibration_apply_open_percent(cal, 60);
  EXPECT_GE(out, 0);
  EXPECT_LE(out, 100);
}
