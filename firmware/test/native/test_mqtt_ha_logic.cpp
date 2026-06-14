#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

#include "json_field_parse.h"
#include "mqtt_ha_logic.h"

namespace {

String load_golden_json(const char *path) {
  std::ifstream in(path);
  std::stringstream ss;
  ss << in.rdbuf();
  return String(ss.str().c_str());
}

}  // namespace

TEST(MqttHaLogic, TopicsMatchGoldenShape) {
  const String golden = load_golden_json("firmware/test/golden/mqtt/topic_shapes.json");
  ASSERT_GT(golden.length(), 0u);

  const String discoveryPrefix = parse_json_string("\"discovery_prefix\"", golden);
  const String telemetryPrefix = parse_json_string("\"telemetry_prefix\"", golden);
  const String device = parse_json_string("\"device\"", golden);
  ASSERT_STREQ(mqtt_ha_logic_discovery_prefix(), discoveryPrefix.c_str());
  ASSERT_EQ(device, "BALANSUN_TEST");

  EXPECT_STREQ(mqtt_ha_logic_state_topic(telemetryPrefix.c_str(), device.c_str()).c_str(),
               parse_json_string("\"state_topic\"", golden).c_str());
  EXPECT_STREQ(mqtt_ha_logic_availability_topic(telemetryPrefix.c_str(), device.c_str()).c_str(),
               parse_json_string("\"availability_topic\"", golden).c_str());
  EXPECT_STREQ(mqtt_ha_logic_discovery_topic(discoveryPrefix.c_str(), "sensor", device.c_str(), "Pw").c_str(),
               parse_json_string("\"discovery_sensor_pw\"", golden).c_str());
  EXPECT_STREQ(mqtt_ha_logic_command_topic_triac(telemetryPrefix.c_str(), device.c_str()).c_str(),
               parse_json_string("\"command_triac\"", golden).c_str());
  EXPECT_STREQ(mqtt_ha_logic_sensor_discovery_name(device.c_str(), "Pw").c_str(),
               parse_json_string("\"sensor_discovery_name_pw\"", golden).c_str());
}

TEST(MqttHaLogic, DiscoveryGoldenName) {
  const String disc = load_golden_json("firmware/test/golden/mqtt/discovery_sensor_pw.json");
  EXPECT_STREQ(mqtt_ha_logic_sensor_discovery_name("BALANSUN_TEST", "Pw").c_str(),
               parse_json_string("\"name\"", disc).c_str());
}
