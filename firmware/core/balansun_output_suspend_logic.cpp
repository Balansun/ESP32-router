#include "balansun_output_suspend_logic.h"

BalansunOutputSuspendResult balansun_output_suspend_eval(const BalansunOutputSuspendInput &in) {
  BalansunOutputSuspendResult out{};
  if (!balansun_product_caps_has(in.caps, BalansunCap::SurplusRegulation)) {
    return out;
  }
  if (in.safety_lockout_active) {
    out.active = true;
    out.reason = BalansunOutputSuspendReason::SafetyLockout;
    return out;
  }
  if (balansun_product_caps_has(in.caps, BalansunCap::VacationMode) && in.vacation_active) {
    out.active = true;
    out.reason = BalansunOutputSuspendReason::Vacation;
    return out;
  }
  if (in.triac_off_when_source_stale && in.source_stale) {
    out.active = true;
    out.reason = BalansunOutputSuspendReason::SourceStale;
    return out;
  }
  if (in.self_test_blocks_outputs ||
      (in.commissioning_blocks_outputs && in.self_test_running)) {
    out.active = true;
    out.reason = BalansunOutputSuspendReason::Commissioning;
    return out;
  }
  return out;
}

const char *balansun_output_suspend_reason_wire(BalansunOutputSuspendReason reason) {
  switch (reason) {
    case BalansunOutputSuspendReason::SafetyLockout:
      return "safety_lockout";
    case BalansunOutputSuspendReason::Vacation:
      return "vacation";
    case BalansunOutputSuspendReason::SourceStale:
      return "source_stale";
    case BalansunOutputSuspendReason::Commissioning:
      return "commissioning";
    case BalansunOutputSuspendReason::None:
    default:
      return "none";
  }
}
