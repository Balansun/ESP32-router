#pragma once

/* pmqtt_logic.h — Classify/parse Pmqtt MQTT JSON payloads (Pw schema vs house snapshot). */

#include <string>

enum class PmqttPayloadKind { None, PwPf, HouseSnapshot };

struct PmqttPwReading {
  int house_active_import_w = 0;
  int house_active_export_w = 0;
  int house_apparent_import_va = 0;
  int house_apparent_export_va = 0;
  float pf = 1.0f;
};

PmqttPayloadKind pmqtt_logic_classify_payload(const std::string &json);

bool pmqtt_logic_parse_pw_schema(const std::string &json, const std::string &schema, PmqttPwReading &out);
