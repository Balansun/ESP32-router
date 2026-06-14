#pragma once

#include <string>

/** Home Assistant MQTT integration default discovery prefix (not the router state/cmd prefix). */
const char *mqtt_ha_logic_discovery_prefix();

std::string mqtt_ha_logic_state_topic(const std::string &prefix, const std::string &device_name);
std::string mqtt_ha_logic_availability_topic(const std::string &prefix, const std::string &device_name);
std::string mqtt_ha_logic_discovery_topic(const std::string &prefix, const std::string &component,
                                            const std::string &device_name, const std::string &object_name);
std::string mqtt_ha_logic_command_topic_triac(const std::string &prefix, const std::string &device_name);
std::string mqtt_ha_logic_sensor_discovery_name(const std::string &device_name, const std::string &field_name);
