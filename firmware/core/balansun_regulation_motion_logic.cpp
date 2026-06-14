#include "balansun_regulation_motion_logic.h"

static int abs_i(int v) { return v < 0 ? -v : v; }

BalansunRegulationMotion balansun_regulation_motion_compute(const BalansunRegulationMotionInput &in) {
  if (!balansun_product_caps_has(in.caps, BalansunCap::SurplusRegulation)) {
    return BalansunRegulationMotion::Idle;
  }
  if (in.suspend_active || !in.schedule_allows_surplus) {
    return BalansunRegulationMotion::Idle;
  }
  if (in.new_open_percent <= in.floor_percent) {
    return BalansunRegulationMotion::AtMinimum;
  }
  if (in.new_open_percent >= in.ceiling_percent) {
    return BalansunRegulationMotion::AtMaximum;
  }
  const int delta = in.new_open_percent - in.prev_open_percent;
  if (abs_i(delta) <= in.deadband_percent) {
    return BalansunRegulationMotion::Idle;
  }
  return delta > 0 ? BalansunRegulationMotion::Increasing : BalansunRegulationMotion::Decreasing;
}

const char *balansun_regulation_motion_wire(BalansunRegulationMotion motion) {
  switch (motion) {
    case BalansunRegulationMotion::Increasing:
      return "increasing";
    case BalansunRegulationMotion::Decreasing:
      return "decreasing";
    case BalansunRegulationMotion::AtMinimum:
      return "at_minimum";
    case BalansunRegulationMotion::AtMaximum:
      return "at_maximum";
    case BalansunRegulationMotion::Idle:
    default:
      return "idle";
  }
}
