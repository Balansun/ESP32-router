#pragma once

#include <cstdint>
#include <string>

/** MQTT JSON config payloads use schema_version 2. */
constexpr int kMqttJsonConfigSchemaMin = 2;
constexpr int kMqttJsonConfigSchemaMax = 2;

bool mqtt_ha_json_config_schema_valid(int schema);

struct MqttActionConfigPatch {
  bool ok = false;
  int threshold_w = -1;
  int hour_end = -1;
  int power_min_w = -1;
  int power_max_w = -1;
  /** 1=off, 2=on, 3=power; -1 = unchanged. */
  int mode_type = -1;
  std::string error;
};

struct MqttSiteConfigPatch {
  bool ok = false;
  int max_routed_w = -1;
  bool has_triac_off_when_source_stale = false;
  bool triac_off_when_source_stale = false;
  bool has_triac_backoff_when_heater_idle = false;
  bool triac_backoff_when_heater_idle = false;
  int action_daily_cap_index = -1;
  uint32_t action_daily_cap_wh = 0;
  std::string error;
};

struct MqttVacationConfigPatch {
  bool ok = false;
  bool has_vacation_enabled = false;
  bool vacation_enabled = false;
  int64_t vacation_end_epoch = -1;
  std::string error;
};

MqttActionConfigPatch mqtt_ha_json_config_parse_action(const std::string &json);
MqttSiteConfigPatch mqtt_ha_json_config_parse_site(const std::string &json);
MqttVacationConfigPatch mqtt_ha_json_config_parse_vacation(const std::string &json);
