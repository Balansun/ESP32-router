#include "balansun_config_cap_logic.h"

#include "balansun_product_profile.h"
#include "balansun_source_logic.h"

#include <cstring>

const BalansunConfigKeyCapRule kConfigKeyCapRules[] = {
    {"ext_peer_ip", BalansunConfigCapRuleKind::MeterWire, {.meter_wire = "BalansunPeer"}},
    {"ext_peer_port", BalansunConfigCapRuleKind::MeterWire, {.meter_wire = "BalansunPeer"}},
    {"ext_peer_path", BalansunConfigCapRuleKind::MeterWire, {.meter_wire = "BalansunPeer"}},
    {"ext_protocol", BalansunConfigCapRuleKind::MeterWire, {.meter_wire = "BalansunPeer"}},
    {"enphase_user", BalansunConfigCapRuleKind::MeterWire, {.meter_wire = "Enphase"}},
    {"enphase_password", BalansunConfigCapRuleKind::MeterWire, {.meter_wire = "Enphase"}},
    {"meter_channel", BalansunConfigCapRuleKind::MeterWire, {.meter_wire = "Enphase"}},
    {"enphase_serial", BalansunConfigCapRuleKind::MeterWire, {.meter_wire = "Enphase"}},
    {"pmqtt_topic", BalansunConfigCapRuleKind::MeterWire, {.meter_wire = "Pmqtt"}},
    {"pmqtt_schema", BalansunConfigCapRuleKind::MeterWire, {.meter_wire = "Pmqtt"}},
    {"pmqtt_bindings", BalansunConfigCapRuleKind::MeterWire, {.meter_wire = "Pmqtt"}},
    {"jsy_mk333_serial_baud", BalansunConfigCapRuleKind::MeterWire, {.meter_wire = "JsyMk333"}},
    {"calib_u", BalansunConfigCapRuleKind::MeterWire, {.meter_wire = "Analog"}},
    {"calib_i", BalansunConfigCapRuleKind::MeterWire, {.meter_wire = "Analog"}},
    {"tempo_rte_enabled", BalansunConfigCapRuleKind::MeterWire, {.meter_wire = "Linky"}},
    {"expert_regulation_mode", BalansunConfigCapRuleKind::ProductCap, {.cap = BalansunCap::SurplusRegulation}},
    {"regulation_gain", BalansunConfigCapRuleKind::ProductCap, {.cap = BalansunCap::SurplusRegulation}},
    {"hunting_reversal_threshold", BalansunConfigCapRuleKind::ProductCap, {.cap = BalansunCap::SurplusRegulation}},
    {"hunting_window_min", BalansunConfigCapRuleKind::ProductCap, {.cap = BalansunCap::SurplusRegulation}},
    {"max_routed_w", BalansunConfigCapRuleKind::ProductCap, {.cap = BalansunCap::SurplusRegulation}},
    {"triac_off_when_source_stale", BalansunConfigCapRuleKind::ProductCap, {.cap = BalansunCap::SurplusRegulation}},
    {"triac_backoff_when_heater_idle", BalansunConfigCapRuleKind::ProductCap, {.cap = BalansunCap::SurplusRegulation}},
    {"commissioning_blocks_outputs", BalansunConfigCapRuleKind::ProductCap, {.cap = BalansunCap::SurplusRegulation}},
    {"action_daily_cap_wh", BalansunConfigCapRuleKind::ProductCap, {.cap = BalansunCap::SurplusRegulation}},
    {"triac_override_max_temp_c", BalansunConfigCapRuleKind::ProductCap, {.cap = BalansunCap::SurplusRegulation}},
    {"pin_triac_dim", BalansunConfigCapRuleKind::ProductCap, {.cap = BalansunCap::SurplusRegulation}},
    {"pin_zero_cross", BalansunConfigCapRuleKind::ProductCap, {.cap = BalansunCap::SurplusRegulation}},
    {"probe_second_name", BalansunConfigCapRuleKind::ProductCap, {.cap = BalansunCap::SurplusRegulation}},
    {"triac_cal_enabled", BalansunConfigCapRuleKind::ProductCap, {.cap = BalansunCap::SurplusRegulation}},
    {"triac_calibration", BalansunConfigCapRuleKind::ProductCap, {.cap = BalansunCap::SurplusRegulation}},
};

const size_t kConfigKeyCapRulesCount = sizeof(kConfigKeyCapRules) / sizeof(kConfigKeyCapRules[0]);

static bool default_meter_wire_supported(const char *wire) {
  if (!wire || !wire[0]) return false;
  const char *norm = balansun_source_logic_normalize_wire_cstr(wire);
  return balansun_source_logic_parse_wire(norm) != SourceId::Unknown;
}

BalansunConfigCapContext balansun_config_cap_context_default() {
  BalansunConfigCapContext ctx{};
  ctx.product_caps = balansun_product_caps_for_role(BALANSUN_ROLE);
  ctx.meter_wire_supported = nullptr;
  return ctx;
}

bool balansun_config_meter_wire_supported(const BalansunConfigCapContext &ctx, const char *wire) {
  if (ctx.meter_wire_supported) return ctx.meter_wire_supported(wire);
  return default_meter_wire_supported(wire);
}

static void copy_missing_cap(char missing_cap_out[64], const char *wire) {
  strncpy(missing_cap_out, wire, 63);
  missing_cap_out[63] = '\0';
}

static const BalansunConfigKeyCapRule *find_rule(const char *key) {
  if (!key) return nullptr;
  for (size_t i = 0; i < kConfigKeyCapRulesCount; ++i) {
    if (strcmp(kConfigKeyCapRules[i].key, key) == 0) return &kConfigKeyCapRules[i];
  }
  return nullptr;
}

static bool safety_lockout_denied_key(const char *key) {
  if (!key || !key[0]) return false;
  static const char *const kDenied[] = {
      "commissioning_blocks_outputs",
      "triac_off_when_source_stale",
      "expert_regulation_mode",
      "regulation_gain",
      "hunting_reversal_threshold",
      "hunting_window_min",
      "max_routed_w",
      "triac_backoff_when_heater_idle",
      "action_daily_cap_wh",
      "triac_override_max_temp_c",
      "triac_cal_enabled",
      "triac_calibration",
      "pwm_mode",
      "pwm_gpio",
      "pin_triac_dim",
      "pin_zero_cross",
      "vacation_enabled",
      "vacation_end_epoch",
  };
  for (size_t i = 0; i < sizeof(kDenied) / sizeof(kDenied[0]); ++i) {
    if (strcmp(key, kDenied[i]) == 0) return true;
  }
  if (strncmp(key, "triac_cal_", 10) == 0) return true;
  return false;
}

bool balansun_config_key_blocked_by_safety_lockout(const char *key) {
  return safety_lockout_denied_key(key);
}

bool balansun_config_key_allowed(const BalansunConfigCapContext &ctx, const char *key, const char *value_cstr,
                              char missing_cap_out[64]) {
  missing_cap_out[0] = '\0';
  if (!key) return true;

  if (strcmp(key, "source") == 0) {
    if (!balansun_config_meter_wire_supported(ctx, value_cstr)) {
      copy_missing_cap(missing_cap_out, "unsupported_source");
      return false;
    }
    return true;
  }

  if (strcmp(key, "pwm_mode") == 0 && value_cstr && strcmp(value_cstr, "follow_triac") == 0) {
    if (!balansun_product_caps_has(ctx.product_caps, BalansunCap::SurplusRegulation)) {
      copy_missing_cap(missing_cap_out, balansun_product_cap_wire(BalansunCap::SurplusRegulation));
      return false;
    }
    return true;
  }

  const BalansunConfigKeyCapRule *rule = find_rule(key);
  if (!rule) return true;

  if (rule->kind == BalansunConfigCapRuleKind::ProductCap) {
    if (!balansun_product_caps_has(ctx.product_caps, rule->u.cap)) {
      copy_missing_cap(missing_cap_out, balansun_product_cap_wire(rule->u.cap));
      return false;
    }
    return true;
  }

  if (!balansun_config_meter_wire_supported(ctx, rule->u.meter_wire)) {
    copy_missing_cap(missing_cap_out, "unsupported_meter_field");
    return false;
  }
  return true;
}

const char *balansun_config_cap_disabled_message(const char *missing_cap) {
  if (!missing_cap) return "Capability is not available on this firmware profile";
  if (strcmp(missing_cap, "unsupported_source") == 0) {
    return "Selected meter source is not compiled in this firmware profile";
  }
  if (strcmp(missing_cap, "unsupported_meter_field") == 0) {
    return "This meter-specific setting is not available on this firmware profile";
  }
  if (strcmp(missing_cap, "triac") == 0) return "Triac dimming is not available on this firmware profile";
  if (strcmp(missing_cap, "surplus_regulation") == 0) {
    return "Surplus regulation is not available on this firmware profile";
  }
  if (strcmp(missing_cap, "multi_action") == 0) {
    return "Multiple routing actions are not available on this firmware profile";
  }
  if (strcmp(missing_cap, "source_test_inject") == 0) {
    return "Source test inject is not available on this firmware profile";
  }
  if (strcmp(missing_cap, "self_test_triac") == 0) {
    return "Triac self-test is not available on this firmware profile";
  }
  if (strcmp(missing_cap, "safety_lockout") == 0) {
    return "Routing is disabled until commissioning self-test passes (critical hardware check failed)";
  }
  return "Capability is not available on this firmware profile";
}
