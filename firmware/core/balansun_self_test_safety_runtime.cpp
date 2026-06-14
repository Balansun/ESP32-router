#include "balansun_self_test_safety_runtime.h"

#include "balansun_globals.h"
#include "balansun_product_caps.h"
#include "balansun_self_test.h"

BalansunSelfTestSafetyResult balansun_self_test_safety_eval_now() {
  BalansunSelfTestSafetyInput in{};
  in.caps = balansun_product_caps_compile_time();
  in.self_test = &g_self_test;
  in.triac_off_when_source_stale = triacOffWhenSourceStale;
  in.self_test_running = balansun_self_test_is_running();
  return balansun_self_test_safety_eval(in);
}

bool balansun_api_safety_lockout_active() { return balansun_self_test_safety_eval_now().lockout_active; }
