#include <gtest/gtest.h>

#include "mqtt_ha_json_config_logic.h"

TEST(MqttJsonConfig, ActionSchema2ScheduleFields) {
  const MqttActionConfigPatch p =
      mqtt_ha_json_config_parse_action(R"({"schema_version":2,"hour_end":1800,"power_min_w":500,"mode":"power"})");
  ASSERT_TRUE(p.ok);
  EXPECT_EQ(p.hour_end, 1800);
  EXPECT_EQ(p.power_min_w, 500);
  EXPECT_EQ(p.mode_type, 3);
}

TEST(MqttJsonConfig, SiteTriacStaleAndCap) {
  const MqttSiteConfigPatch p = mqtt_ha_json_config_parse_site(
      R"({"schema_version":2,"max_routed_w":4000,"triac_off_when_source_stale":true,"triac_backoff_when_heater_idle":true,"action_index":1,"daily_cap_wh":8000})");
  ASSERT_TRUE(p.ok);
  EXPECT_EQ(p.max_routed_w, 4000);
  EXPECT_TRUE(p.has_triac_off_when_source_stale);
  EXPECT_TRUE(p.triac_off_when_source_stale);
  EXPECT_TRUE(p.has_triac_backoff_when_heater_idle);
  EXPECT_TRUE(p.triac_backoff_when_heater_idle);
  EXPECT_EQ(p.action_daily_cap_index, 1);
  EXPECT_EQ(p.action_daily_cap_wh, 8000u);
}

TEST(MqttJsonConfig, VacationPatch) {
  const MqttVacationConfigPatch p =
      mqtt_ha_json_config_parse_vacation(R"({"schema_version":2,"vacation_enabled":true,"vacation_end_epoch":2000000000})");
  ASSERT_TRUE(p.ok);
  EXPECT_TRUE(p.has_vacation_enabled);
  EXPECT_TRUE(p.vacation_enabled);
  EXPECT_EQ(p.vacation_end_epoch, 2000000000);
}

TEST(MqttJsonConfig, RejectsInvalidSchema) {
  const MqttActionConfigPatch p = mqtt_ha_json_config_parse_action(R"({"schema_version":9})");
  EXPECT_FALSE(p.ok);
}

TEST(MqttJsonConfig, SiteCapRequiresDailyCapWhWithActionIndex) {
  const MqttSiteConfigPatch p = mqtt_ha_json_config_parse_site(
      R"({"schema_version":2,"action_index":1})");
  EXPECT_FALSE(p.ok);
  EXPECT_NE(p.error.find("daily_cap_wh"), std::string::npos);
}

TEST(MqttJsonConfig, SiteCapRejectsOutOfRangeActionIndex) {
  const MqttSiteConfigPatch p = mqtt_ha_json_config_parse_site(
      R"({"schema_version":2,"action_index":20,"daily_cap_wh":100})");
  EXPECT_FALSE(p.ok);
  EXPECT_NE(p.error.find("action_index"), std::string::npos);
}

TEST(MqttJsonConfig, SiteCapAllowsIndexZero) {
  const MqttSiteConfigPatch p = mqtt_ha_json_config_parse_site(
      R"({"schema_version":2,"action_index":0,"daily_cap_wh":0})");
  ASSERT_TRUE(p.ok);
  EXPECT_EQ(p.action_daily_cap_index, 0);
  EXPECT_EQ(p.action_daily_cap_wh, 0u);
}

TEST(MqttJsonConfig, ActionValidationErrors) {
  EXPECT_FALSE(mqtt_ha_json_config_parse_action("").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":2,"threshold_w":-1})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":2,"hour_end":9999})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":2,"power_min_w":60000})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":2,"mode":"invalid"})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":2})").ok);
  EXPECT_TRUE(mqtt_ha_json_config_schema_valid(2));
  EXPECT_FALSE(mqtt_ha_json_config_schema_valid(1));
}

TEST(MqttJsonConfig, SiteValidationErrors) {
  EXPECT_FALSE(mqtt_ha_json_config_parse_site("").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_site(R"({"schema_version":2,"max_routed_w":-1})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_site(R"({"schema_version":2})").ok);
}

TEST(MqttJsonConfig, VacationValidationErrors) {
  EXPECT_FALSE(mqtt_ha_json_config_parse_vacation(R"({"schema_version":2})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_vacation(R"({"schema_version":2,"vacation_end_epoch":-1})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_vacation("").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_vacation(R"({"schema_version":1})").ok);
}

TEST(MqttJsonConfig, ParsesThresholdAndPowerMax) {
  const auto a = mqtt_ha_json_config_parse_action(
      R"({"schema_version":2,"threshold_w":1200,"power_max_w":4000,"mode":"off"})");
  ASSERT_TRUE(a.ok);
  EXPECT_EQ(a.threshold_w, 1200);
  EXPECT_EQ(a.power_max_w, 4000);
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":2,"power_max_w":60000})").ok);
}

TEST(MqttJsonConfig, SiteBoolFalse) {
  const auto s = mqtt_ha_json_config_parse_site(
      R"({"schema_version":2,"triac_off_when_source_stale":false})");
  ASSERT_TRUE(s.ok);
  EXPECT_FALSE(s.triac_off_when_source_stale);
  EXPECT_FALSE(mqtt_ha_json_config_parse_site(R"({"schema_version":1})").ok);
}

TEST(MqttJsonConfig, RejectsNonBooleanField) {
  EXPECT_FALSE(mqtt_ha_json_config_parse_site(
      R"({"schema_version":2,"triac_off_when_source_stale":"maybe"})").ok);
}

TEST(MqttJsonConfig, RejectsNonObjectRoot) {
  EXPECT_FALSE(mqtt_ha_json_config_parse_action("[]").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_site("").ok);
}

TEST(MqttJsonConfig, ActionModeOn) {
  const auto a = mqtt_ha_json_config_parse_action(R"({"schema_version":2,"mode":"on"})");
  ASSERT_TRUE(a.ok);
  EXPECT_EQ(a.mode_type, 2);
}

TEST(MqttJsonConfig, VacationEnabledFalse) {
  const auto v = mqtt_ha_json_config_parse_vacation(
      R"({"schema_version":2,"vacation_enabled":false,"vacation_end_epoch":0})");
  ASSERT_TRUE(v.ok);
  EXPECT_TRUE(v.has_vacation_enabled);
  EXPECT_FALSE(v.vacation_enabled);
}

TEST(MqttJsonConfig, ActionThresholdAtMaxBoundary) {
  const auto a = mqtt_ha_json_config_parse_action(R"({"schema_version":2,"threshold_w":50000})");
  ASSERT_TRUE(a.ok);
  EXPECT_EQ(a.threshold_w, 50000);
}

TEST(MqttJsonConfig, ActionHourEndOnly) {
  const auto a = mqtt_ha_json_config_parse_action(R"({"schema_version":2,"hour_end":0})");
  ASSERT_TRUE(a.ok);
  EXPECT_EQ(a.hour_end, 0);
}

TEST(MqttJsonConfig, ActionPowerMinAtNegativeBoundary) {
  const auto a = mqtt_ha_json_config_parse_action(
      R"({"schema_version":2,"power_min_w":-50000,"mode":"power"})");
  ASSERT_TRUE(a.ok);
  EXPECT_EQ(a.power_min_w, -50000);
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":2,"power_min_w":-50001})").ok);
}

TEST(MqttJsonConfig, ActionModeOff) {
  const auto a = mqtt_ha_json_config_parse_action(R"({"schema_version":2,"mode":"off"})");
  ASSERT_TRUE(a.ok);
  EXPECT_EQ(a.mode_type, 1);
}

TEST(MqttJsonConfig, RejectsMalformedIntAndStringFields) {
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":"two"})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":2,"threshold_w":})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":2,"mode":"on)").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_site(R"({"schema_version":2,"max_routed_w":})").ok);
}

TEST(MqttJsonConfig, SiteMaxRoutedAtBoundaryAndNegativeCap) {
  const auto ok = mqtt_ha_json_config_parse_site(R"({"schema_version":2,"max_routed_w":20000})");
  ASSERT_TRUE(ok.ok);
  EXPECT_EQ(ok.max_routed_w, 20000);
  EXPECT_FALSE(mqtt_ha_json_config_parse_site(
                   R"({"schema_version":2,"action_index":0,"daily_cap_wh":-1})")
                   .ok);
}

TEST(MqttJsonConfig, SiteBackoffOnly) {
  const auto s = mqtt_ha_json_config_parse_site(
      R"({"schema_version":2,"triac_backoff_when_heater_idle":false})");
  ASSERT_TRUE(s.ok);
  EXPECT_TRUE(s.has_triac_backoff_when_heater_idle);
  EXPECT_FALSE(s.triac_backoff_when_heater_idle);
}

TEST(MqttJsonConfig, VacationEndEpochOnly) {
  const auto v = mqtt_ha_json_config_parse_vacation(R"({"schema_version":2,"vacation_end_epoch":0})");
  ASSERT_TRUE(v.ok);
  EXPECT_FALSE(v.has_vacation_enabled);
  EXPECT_EQ(v.vacation_end_epoch, 0);
}

TEST(MqttJsonConfig, RejectsSchemaAboveMax) {
  EXPECT_FALSE(mqtt_ha_json_config_schema_valid(3));
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":3,"threshold_w":1})").ok);
}

TEST(MqttJsonConfig, ActionPowerMaxOnly) {
  const auto a = mqtt_ha_json_config_parse_action(R"({"schema_version":2,"power_max_w":3000})");
  ASSERT_TRUE(a.ok);
  EXPECT_EQ(a.power_max_w, 3000);
}

TEST(MqttJsonConfig, RejectsMissingSchemaVersionField) {
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"threshold_w":100})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_site(R"({"max_routed_w":1000})").ok);
}

TEST(MqttJsonConfig, RejectsIntFieldWithoutColon) {
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version"2})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":2,"threshold_w"1200})").ok);
}

TEST(MqttJsonConfig, SiteStaleTrueOnly) {
  const auto s = mqtt_ha_json_config_parse_site(
      R"({"schema_version":2,"triac_off_when_source_stale":true})");
  ASSERT_TRUE(s.ok);
  EXPECT_TRUE(s.triac_off_when_source_stale);
}

TEST(MqttJsonConfig, SiteCapAndRangeErrors) {
  EXPECT_FALSE(mqtt_ha_json_config_parse_site(R"({"schema_version":2,"max_routed_w":-1})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_site(R"({"schema_version":2,"max_routed_w":20001})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_site(R"({"schema_version":2,"action_index":0})").ok);
  EXPECT_FALSE(
      mqtt_ha_json_config_parse_site(R"({"schema_version":2,"action_index":20,"daily_cap_wh":100})").ok);
  const auto cap = mqtt_ha_json_config_parse_site(
      R"({"schema_version":2,"action_index":1,"daily_cap_wh":5000})");
  ASSERT_TRUE(cap.ok);
  EXPECT_EQ(cap.action_daily_cap_index, 1);
  EXPECT_EQ(cap.action_daily_cap_wh, 5000u);
}

TEST(MqttJsonConfig, VacationAndActionRangeErrors) {
  EXPECT_FALSE(mqtt_ha_json_config_parse_vacation(R"({"schema_version":2,"vacation_end_epoch":-1})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_vacation(R"({"schema_version":2})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":2,"hour_end":2401})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":2,"threshold_w":-1})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":2,"power_max_w":50001})").ok);
}

TEST(MqttJsonConfig, TabWhitespaceAndUpperBoundErrors) {
  EXPECT_TRUE(mqtt_ha_json_config_parse_action(
                 R"({"schema_version":	2,"threshold_w":	100})")
      .ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":2,"threshold_w":50001})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":2,"hour_end":-1})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_site(
                   R"({"schema_version":2,"action_index":-1,"daily_cap_wh":100})")
                   .ok);
  EXPECT_TRUE(mqtt_ha_json_config_parse_site(
                 R"({"schema_version":2,"triac_off_when_source_stale":	true})")
      .ok);
}

TEST(MqttJsonConfig, ActionOkWithPowerMinOnly) {
  const auto a = mqtt_ha_json_config_parse_action(R"({"schema_version":2,"power_min_w":100})");
  ASSERT_TRUE(a.ok);
  EXPECT_EQ(a.power_min_w, 100);
}

TEST(MqttJsonConfig, JsonFinderBranches) {
  EXPECT_FALSE(mqtt_ha_json_config_parse_action("[]").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":true})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":2,"mode":"maybe"})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":2,"mode":})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_action(R"({"schema_version":2,"threshold_w":})").ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_site(R"({"schema_version":2,"triac_off_when_source_stale":"yes"})").ok);
  EXPECT_TRUE(mqtt_ha_json_config_parse_site(
                 R"({"schema_version":2,"triac_backoff_when_heater_idle":false})")
      .ok);
  EXPECT_TRUE(mqtt_ha_json_config_parse_vacation(R"({"schema_version":2,"vacation_enabled":false})").ok);
  EXPECT_TRUE(mqtt_ha_json_config_parse_action(
                 R"({"schema_version":2,"mode":"off","threshold_w":100})")
      .ok);
  EXPECT_FALSE(mqtt_ha_json_config_parse_site(R"({"schema_version":2,"max_routed_w":})").ok);
}
