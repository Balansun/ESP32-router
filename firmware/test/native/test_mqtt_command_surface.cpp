#include <gtest/gtest.h>

#include "mqtt_ha_command_logic.h"

TEST(MqttCommandSurface, TriacAuto) {
  MqttTriacCmd cmd{};
  ASSERT_TRUE(mqtt_ha_command_parse_triac("AUTO", &cmd));
  EXPECT_EQ(cmd.kind, MqttTriacCmdKind::Auto);
}

TEST(MqttCommandSurface, TriacPercent) {
  MqttTriacCmd cmd{};
  ASSERT_TRUE(mqtt_ha_command_parse_triac("42", &cmd));
  EXPECT_EQ(cmd.kind, MqttTriacCmdKind::FixedPercent);
  EXPECT_EQ(cmd.percent, 42);
}

TEST(MqttCommandSurface, TriacRejectsExpertJson) {
  MqttTriacCmd cmd{};
  EXPECT_FALSE(mqtt_ha_command_parse_triac(R"({"periods":[{"start":0}]})", &cmd));
  EXPECT_TRUE(mqtt_ha_command_is_json_like_payload(R"({"thresholds":[]})"));
}

TEST(MqttCommandSurface, TriacRejectsNonNumericGarbage) {
  MqttTriacCmd cmd{};
  EXPECT_FALSE(mqtt_ha_command_parse_triac("AUTOx", &cmd));
  EXPECT_FALSE(mqtt_ha_command_parse_triac("", &cmd));
}

TEST(MqttCommandSurface, ActionOnOffAuto) {
  MqttActionCmdKind kind = MqttActionCmdKind::Invalid;
  ASSERT_TRUE(mqtt_ha_command_parse_action("ON", &kind));
  EXPECT_EQ(kind, MqttActionCmdKind::On);
  ASSERT_TRUE(mqtt_ha_command_parse_action("off", &kind));
  EXPECT_EQ(kind, MqttActionCmdKind::Off);
  ASSERT_TRUE(mqtt_ha_command_parse_action("Auto", &kind));
  EXPECT_EQ(kind, MqttActionCmdKind::Auto);
}

TEST(MqttCommandSurface, ActionRejectsExpertJson) {
  MqttActionCmdKind kind = MqttActionCmdKind::On;
  EXPECT_FALSE(mqtt_ha_command_parse_action(R"({"mode":"AUTO"})", &kind));
}

TEST(MqttCommandSurface, TriacPercentClampedAndNullOut) {
  MqttTriacCmd cmd{};
  ASSERT_TRUE(mqtt_ha_command_parse_triac("150", &cmd));
  EXPECT_EQ(cmd.percent, 100);
  EXPECT_FALSE(mqtt_ha_command_parse_triac("AUTO", nullptr));
  EXPECT_FALSE(mqtt_ha_command_is_json_like_payload("  "));
}

TEST(MqttCommandSurface, ActionParseNullAndInvalid) {
  EXPECT_FALSE(mqtt_ha_command_parse_action("ON", nullptr));
  MqttActionCmdKind kind = MqttActionCmdKind::On;
  EXPECT_FALSE(mqtt_ha_command_parse_action("ON?", &kind));
}

TEST(MqttCommandSurface, TriacPercentBoundaries) {
  MqttTriacCmd cmd{};
  ASSERT_TRUE(mqtt_ha_command_parse_triac("0", &cmd));
  EXPECT_EQ(cmd.percent, 0);
  ASSERT_TRUE(mqtt_ha_command_parse_triac("101", &cmd));
  EXPECT_EQ(cmd.percent, 100);
  EXPECT_FALSE(mqtt_ha_command_parse_triac("12a", &cmd));
  EXPECT_TRUE(mqtt_ha_command_is_json_like_payload("[1,2]"));
}

TEST(MqttCommandSurface, ActionUnknownToken) {
  MqttActionCmdKind kind = MqttActionCmdKind::On;
  EXPECT_FALSE(mqtt_ha_command_parse_action("HOLD", &kind));
  EXPECT_EQ(kind, MqttActionCmdKind::Invalid);
}

TEST(MqttCommandSurface, TrimsTrailingWhitespace) {
  MqttTriacCmd cmd{};
  ASSERT_TRUE(mqtt_ha_command_parse_triac("75  \t", &cmd));
  EXPECT_EQ(cmd.percent, 75);
  MqttActionCmdKind kind = MqttActionCmdKind::Invalid;
  ASSERT_TRUE(mqtt_ha_command_parse_action("  off \n", &kind));
  EXPECT_EQ(kind, MqttActionCmdKind::Off);
  EXPECT_FALSE(mqtt_ha_command_parse_action("   ", &kind));
}
