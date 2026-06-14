#pragma once

#include <cstdint>

struct TriacCalPoint {
  uint8_t duty_pct = 0;
  uint16_t measured_w = 0;
};

struct TriacCalibrationTable {
  bool enabled = false;
  TriacCalPoint points[3];
};

/** Scale open percent 0..100 using calibration curve (identity when disabled). */
int balansun_triac_calibration_apply_open_percent(const TriacCalibrationTable &cal, int open_percent);
