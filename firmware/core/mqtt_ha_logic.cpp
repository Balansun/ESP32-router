#include "mqtt_ha_logic.h"

const char *mqtt_ha_logic_discovery_prefix() { return "homeassistant"; }

std::string mqtt_ha_logic_state_topic(const std::string &prefix, const std::string &device_name) {
  return prefix + "/" + device_name + "_state";
}

std::string mqtt_ha_logic_availability_topic(const std::string &prefix, const std::string &device_name) {
  return prefix + "/" + device_name + "/availability";
}

std::string mqtt_ha_logic_discovery_topic(const std::string &prefix, const std::string &component,
                                          const std::string &device_name, const std::string &object_name) {
  return prefix + "/" + component + "/" + device_name + "_" + object_name + "/config";
}

std::string mqtt_ha_logic_command_topic_triac(const std::string &prefix, const std::string &device_name) {
  return prefix + "/" + device_name + "/triac/set";
}

std::string mqtt_ha_logic_sensor_discovery_name(const std::string &device_name, const std::string &field_name) {
  return device_name + " " + field_name;
}
