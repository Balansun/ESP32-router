#pragma once

/*
 * balansun_config_cap_logic.h — Config key capability Specification (host-testable).
 * Registry: kConfigKeyCapRules[] maps REST config keys to product caps or meter wires.
 */

#include "balansun_product_caps_logic.h"

#include <cstddef>

enum class BalansunConfigCapRuleKind : uint8_t {
  ProductCap,
  MeterWire,
};

struct BalansunConfigKeyCapRule {
  const char *key;
  BalansunConfigCapRuleKind kind;
  union {
    BalansunCap cap;
    const char *meter_wire;
  } u;
};

extern const BalansunConfigKeyCapRule kConfigKeyCapRules[];
extern const size_t kConfigKeyCapRulesCount;

struct BalansunConfigCapContext {
  BalansunProductCaps product_caps{};
  /** When null, uses balansun_source_logic registry (compile-time meters). */
  bool (*meter_wire_supported)(const char *wire) = nullptr;
};

BalansunConfigCapContext balansun_config_cap_context_default();

bool balansun_config_meter_wire_supported(const BalansunConfigCapContext &ctx, const char *wire);

/** Returns false and fills missing_cap (wire id for 403 JSON). */
bool balansun_config_key_allowed(const BalansunConfigCapContext &ctx, const char *key, const char *value_cstr,
                              char missing_cap_out[64]);

const char *balansun_config_cap_disabled_message(const char *missing_cap);

/** True when safety lockout forbids mutating this config key (routing outputs). */
bool balansun_config_key_blocked_by_safety_lockout(const char *key);
