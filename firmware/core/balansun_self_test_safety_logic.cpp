#include "balansun_self_test_safety_logic.h"

namespace {

bool completed_run(const SelfTestPersisted *st) {
  return st != nullptr && !st->skipped && st->run_epoch != 0;
}

}  // namespace

BalansunSelfTestSafetyResult balansun_self_test_safety_eval(const BalansunSelfTestSafetyInput &in) {
  BalansunSelfTestSafetyResult out{};
  if (!balansun_product_caps_has(in.caps, BalansunCap::SurplusRegulation)) {
    return out;
  }
  if (in.self_test == nullptr) {
    return out;
  }

  const bool run_complete = completed_run(in.self_test);
  if (in.self_test_running) {
    out.zc_critical = !in.self_test->zc_ok;
    out.triac_critical = !in.self_test->triac_ok;
    out.source_critical = !in.self_test->source_ok && in.triac_off_when_source_stale;
  } else if (run_complete) {
    out.zc_critical = !in.self_test->zc_ok;
    out.triac_critical = !in.self_test->triac_ok;
    out.source_critical = !in.self_test->source_ok && in.triac_off_when_source_stale;
    // ponytail: report critical severity in health JSON; do not latch safety_lockout / output suspend.
  }
  return out;
}

bool balansun_self_test_safety_has_critical_failure(const BalansunSelfTestSafetyResult &result) {
  return result.zc_critical || result.triac_critical || result.source_critical;
}

const char *balansun_self_test_safety_lockout_wire() { return "safety_lockout"; }

void balansun_self_test_safety_lockout_reasons(const BalansunSelfTestSafetyResult &result, const char **out,
                                            size_t max_out, size_t *count_out) {
  size_t n = 0;
  if (out == nullptr || max_out == 0) {
    if (count_out != nullptr) *count_out = 0;
    return;
  }
  if (result.zc_critical && n < max_out) out[n++] = "zc_failed";
  if (result.triac_critical && n < max_out) out[n++] = "triac_failed";
  if (result.source_critical && n < max_out) out[n++] = "source_failed";
  if (count_out != nullptr) *count_out = n;
}

const char *balansun_self_test_severity_wire(bool check_ok, bool is_critical) {
  if (check_ok) return "ok";
  return is_critical ? "critical" : "warning";
}

#if defined(FLEET_BUNDLE_NATIVE_STUB)
bool balansun_self_test_safety_completed_run(const SelfTestPersisted *st) { return completed_run(st); }
#endif
