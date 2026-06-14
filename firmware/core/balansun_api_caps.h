#pragma once

/*
 * balansun_api_caps.h — REST capability Guard macros (Chain of Responsibility).
 *
 * Route → cap registry (mutating handlers):
 *   /api/v1/actions/* PUT/PATCH/POST  → SurplusRegulation (+ MultiAction for index ≥ 1)
 *   /api/v1/triac/override POST         → SurplusRegulation (triac intrinsic to router)
 *   /api/v1/gpio PUT (triac pins)       → SurplusRegulation
 *   /api/v1/system/pins PUT (triac)     → SurplusRegulation (via config cap rules)
 *   /api/v1/pwm PUT follow_triac        → SurplusRegulation
 *   /api/v1/health/self-test/run POST → SelfTestTriac
 *   /api/v1/sources/test/inject POST    → SourceTestInject
 *   /api/v1/config PUT/PATCH            → balansun_config_cap_logic Specification
 */

#include "api_util.h"
#include "balansun_product_caps.h"
#include "balansun_self_test_safety_runtime.h"

#define API_SAFETY_LOCKOUT_GUARD() \
  do { \
    if (balansun_api_safety_lockout_active()) { \
      api_error_safety_lockout(server); \
      return; \
    } \
  } while (0)

#define API_CAP_GUARD(cap) \
  do { \
    if (!balansun_product_has_cap(cap)) { \
      api_error_capability_disabled(server, balansun_product_cap_wire(cap)); \
      return; \
    } \
  } while (0)

#define API_CAP_GUARD_WIRE(missing_cap_wire) \
  do { \
    api_error_capability_disabled(server, (missing_cap_wire)); \
    return; \
  } while (0)
