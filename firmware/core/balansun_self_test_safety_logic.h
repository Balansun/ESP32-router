#pragma once

#include "balansun_product_caps_logic.h"
#include "balansun_self_test_persisted.h"

#include <cstddef>

struct BalansunSelfTestSafetyInput {
  BalansunProductCaps caps{};
  const SelfTestPersisted *self_test = nullptr;
  bool triac_off_when_source_stale = false;
  bool self_test_running = false;
};

struct BalansunSelfTestSafetyResult {
  bool lockout_active = false;
  bool zc_critical = false;
  bool triac_critical = false;
  bool source_critical = false;
};

BalansunSelfTestSafetyResult balansun_self_test_safety_eval(const BalansunSelfTestSafetyInput &in);

bool balansun_self_test_safety_has_critical_failure(const BalansunSelfTestSafetyResult &result);

const char *balansun_self_test_safety_lockout_wire();

/** Reason wire strings for health JSON (zc_failed, triac_failed, source_failed). */
void balansun_self_test_safety_lockout_reasons(const BalansunSelfTestSafetyResult &result, const char **out,
                                            size_t max_out, size_t *count_out);

const char *balansun_self_test_severity_wire(bool check_ok, bool is_critical);

#if defined(FLEET_BUNDLE_NATIVE_STUB)
bool balansun_self_test_safety_completed_run(const SelfTestPersisted *st);
#endif
