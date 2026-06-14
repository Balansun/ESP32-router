#pragma once

#include "balansun_product_caps_logic.h"

#include <cstdint>

enum class BalansunRegulationMotion : uint8_t {
  Idle = 0,
  Increasing,
  Decreasing,
  AtMinimum,
  AtMaximum,
};

struct BalansunRegulationMotionInput {
  BalansunProductCaps caps{};
  bool suspend_active = false;
  bool schedule_allows_surplus = true;
  int prev_open_percent = 0;
  int new_open_percent = 0;
  int floor_percent = 0;
  int ceiling_percent = 100;
  int deadband_percent = 1;
};

BalansunRegulationMotion balansun_regulation_motion_compute(const BalansunRegulationMotionInput &in);

const char *balansun_regulation_motion_wire(BalansunRegulationMotion motion);
