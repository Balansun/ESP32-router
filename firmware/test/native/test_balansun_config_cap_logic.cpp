#include <gtest/gtest.h>

#include "balansun_config_cap_logic.h"
#include "balansun_product_profile.h"

static bool jsy_mk194_only_meter(const char *wire) {
  if (!wire || !wire[0]) return false;
  return strcmp(wire, "JsyMk194") == 0;
}

TEST(BalansunConfigCapLogic, MeterGatewayRejectsLinkySource) {
  BalansunConfigCapContext ctx{};
  ctx.product_caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_JSY_MK194_METER);
  ctx.meter_wire_supported = jsy_mk194_only_meter;
  char missing[64] = "";
  EXPECT_FALSE(balansun_config_key_allowed(ctx, "source", "Linky", missing));
  EXPECT_STREQ(missing, "unsupported_source");
  EXPECT_TRUE(balansun_config_key_allowed(ctx, "source", "JsyMk194", missing));
}

TEST(BalansunConfigCapLogic, MeterGatewayRejectsTriacKeys) {
  BalansunConfigCapContext ctx{};
  ctx.product_caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_JSY_MK194_METER);
  ctx.meter_wire_supported = jsy_mk194_only_meter;
  char missing[64] = "";
  EXPECT_FALSE(balansun_config_key_allowed(ctx, "max_routed_w", nullptr, missing));
  EXPECT_STREQ(missing, "surplus_regulation");
  EXPECT_FALSE(balansun_config_key_allowed(ctx, "pin_triac_dim", nullptr, missing));
  EXPECT_STREQ(missing, "surplus_regulation");
}

TEST(BalansunConfigCapLogic, FullRouterAllowsRegulationKeys) {
  BalansunConfigCapContext ctx{};
  ctx.product_caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  char missing[64] = "";
  EXPECT_TRUE(balansun_config_key_allowed(ctx, "max_routed_w", nullptr, missing));
  EXPECT_TRUE(balansun_config_key_allowed(ctx, "pin_triac_dim", nullptr, missing));
}

TEST(BalansunConfigCapLogic, PwmFollowTriacRequiresCap) {
  BalansunConfigCapContext meter{};
  meter.product_caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_JSY_MK194_METER);
  char missing[64] = "";
  EXPECT_FALSE(balansun_config_key_allowed(meter, "pwm_mode", "follow_triac", missing));
  EXPECT_STREQ(missing, "surplus_regulation");

  BalansunConfigCapContext router{};
  router.product_caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  EXPECT_TRUE(balansun_config_key_allowed(router, "pwm_mode", "follow_triac", missing));
}

TEST(BalansunConfigCapLogic, MeterRouterRoleAllowsPwmFollowTriac) {
  BalansunConfigCapContext ctx{};
  ctx.product_caps = balansun_product_caps_for_role(BALANSUN_ROLE_METER_ROUTER);
  char missing[64] = "";
  EXPECT_TRUE(balansun_config_key_allowed(ctx, "pwm_mode", "follow_triac", missing));
}

TEST(BalansunConfigCapLogic, MeterRouterRejectsForeignSource) {
  BalansunConfigCapContext ctx{};
  ctx.product_caps = balansun_product_caps_for_role(BALANSUN_ROLE_METER_ROUTER);
  ctx.meter_wire_supported = jsy_mk194_only_meter;
  char missing[64] = "";
  EXPECT_FALSE(balansun_config_key_allowed(ctx, "source", "Linky", missing));
  EXPECT_STREQ(missing, "unsupported_source");
}

TEST(BalansunConfigCapLogic, DisabledMessageWires) {
  EXPECT_NE(strstr(balansun_config_cap_disabled_message("triac"), "Triac"), nullptr);
  EXPECT_NE(strstr(balansun_config_cap_disabled_message("unsupported_source"), "source"), nullptr);
  EXPECT_NE(strstr(balansun_config_cap_disabled_message("safety_lockout"), "self-test"), nullptr);
}

TEST(BalansunConfigCapLogic, SafetyLockoutDenylist) {
  EXPECT_FALSE(balansun_config_key_blocked_by_safety_lockout(nullptr));
  EXPECT_FALSE(balansun_config_key_blocked_by_safety_lockout(""));
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
  for (const char *key : kDenied) {
    EXPECT_TRUE(balansun_config_key_blocked_by_safety_lockout(key)) << key;
  }
  EXPECT_TRUE(balansun_config_key_blocked_by_safety_lockout("triac_cal_gain"));
  EXPECT_FALSE(balansun_config_key_blocked_by_safety_lockout("router_name"));
  EXPECT_FALSE(balansun_config_key_blocked_by_safety_lockout("mqtt_ip"));
}

static bool any_nonempty_meter(const char *wire) { return wire != nullptr && wire[0] != '\0'; }

TEST(BalansunConfigCapLogic, DefaultContextAndMeterWireRules) {
  const BalansunConfigCapContext defaults = balansun_config_cap_context_default();
  char missing[64] = "";
  EXPECT_TRUE(balansun_config_key_allowed(defaults, "source", "JsyMk194", missing));
  EXPECT_FALSE(balansun_config_key_allowed(defaults, "source", "TotallyUnknownMeter", missing));
  EXPECT_STREQ(missing, "unsupported_source");
  EXPECT_FALSE(balansun_config_meter_wire_supported(defaults, nullptr));
  EXPECT_FALSE(balansun_config_meter_wire_supported(defaults, ""));

  BalansunConfigCapContext meter{};
  meter.product_caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_JSY_MK194_METER);
  meter.meter_wire_supported = jsy_mk194_only_meter;
  EXPECT_FALSE(balansun_config_key_allowed(meter, "ext_peer_ip", nullptr, missing));
  EXPECT_STREQ(missing, "unsupported_meter_field");
  meter.meter_wire_supported = any_nonempty_meter;
  EXPECT_TRUE(balansun_config_key_allowed(meter, "ext_peer_ip", nullptr, missing));
  EXPECT_TRUE(balansun_config_key_allowed(meter, nullptr, nullptr, missing));
  EXPECT_TRUE(balansun_config_key_allowed(meter, "unknown_custom_key", nullptr, missing));
  EXPECT_TRUE(balansun_config_key_allowed(meter, "pwm_mode", "off", missing));
}

TEST(BalansunConfigCapLogic, DisabledMessageVariants) {
  EXPECT_NE(balansun_config_cap_disabled_message(nullptr), nullptr);
  EXPECT_NE(strstr(balansun_config_cap_disabled_message("unsupported_meter_field"), "meter"), nullptr);
  EXPECT_NE(strstr(balansun_config_cap_disabled_message("surplus_regulation"), "regulation"), nullptr);
  EXPECT_NE(strstr(balansun_config_cap_disabled_message("multi_action"), "action"), nullptr);
  EXPECT_NE(strstr(balansun_config_cap_disabled_message("source_test_inject"), "inject"), nullptr);
  EXPECT_NE(strstr(balansun_config_cap_disabled_message("self_test_triac"), "self-test"), nullptr);
  EXPECT_NE(strstr(balansun_config_cap_disabled_message("unknown_cap"), "profile"), nullptr);
}
