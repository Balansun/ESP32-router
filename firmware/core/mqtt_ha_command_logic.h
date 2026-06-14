#pragma once

/*
 * mqtt_ha_command_logic.h — Home Assistant MQTT command payload parsing (string commands only).
 * Rejects upstream expert JSON blobs on triac/action topics (see /en/user-guide/ §E.2).
 */

#include <string>

bool mqtt_ha_command_is_json_like_payload(const std::string &msg);

enum class MqttTriacCmdKind { Invalid, Auto, FixedPercent };

struct MqttTriacCmd {
  MqttTriacCmdKind kind = MqttTriacCmdKind::Invalid;
  int percent = 0;
};

/** Parse triac/set: AUTO or unsigned percent 0–100. */
bool mqtt_ha_command_parse_triac(const std::string &msg, MqttTriacCmd *out);

enum class MqttActionCmdKind { Invalid, On, Off, Auto };

/** Parse action_N/set: ON, OFF, or AUTO. */
bool mqtt_ha_command_parse_action(const std::string &msg, MqttActionCmdKind *out);
