#include "mqtt_ha_json_config_logic.h"

#include <cctype>
#include <cstring>

namespace {

bool find_int_field(const std::string &json, const char *key, int *out) {
  const std::string needle = std::string("\"") + key + "\"";
  size_t pos = json.find(needle);
  if (pos == std::string::npos) return false;
  pos = json.find(':', pos);
  if (pos == std::string::npos) return false;
  pos++;
  while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) pos++;
  size_t end = pos;
  while (end < json.size() && (std::isdigit(static_cast<unsigned char>(json[end])) || json[end] == '-')) end++;
  if (end == pos) return false;
  *out = std::atoi(json.substr(pos, end - pos).c_str());
  return true;
}

bool find_bool_field(const std::string &json, const char *key, bool *out) {
  const std::string needle = std::string("\"") + key + "\"";
  size_t pos = json.find(needle);
  if (pos == std::string::npos) return false;
  pos = json.find(':', pos);
  if (pos == std::string::npos) return false;
  pos++;
  while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) pos++;
  if (json.compare(pos, 4, "true") == 0) {
    *out = true;
    return true;
  }
  if (json.compare(pos, 5, "false") == 0) {
    *out = false;
    return true;
  }
  return false;
}

bool find_string_field(const std::string &json, const char *key, std::string *out) {
  const std::string needle = std::string("\"") + key + "\"";
  size_t pos = json.find(needle);
  if (pos == std::string::npos) return false;
  pos = json.find(':', pos);
  if (pos == std::string::npos) return false;
  pos = json.find('"', pos + 1);
  if (pos == std::string::npos) return false;
  pos++;
  const size_t end = json.find('"', pos);
  if (end == std::string::npos) return false;
  *out = json.substr(pos, end - pos);
  return true;
}

int mode_string_to_type(const std::string &mode) {
  if (mode == "off") return 1;
  if (mode == "on") return 2;
  if (mode == "power") return 3;
  return 0;
}

bool object_root(const std::string &json) {
  return !json.empty() && json.front() == '{';
}

}  // namespace

bool mqtt_ha_json_config_schema_valid(int schema) {
  return schema >= kMqttJsonConfigSchemaMin && schema <= kMqttJsonConfigSchemaMax;
}

MqttActionConfigPatch mqtt_ha_json_config_parse_action(const std::string &json) {
  MqttActionConfigPatch out;
  if (!object_root(json)) {
    out.error = "expected JSON object";
    return out;
  }
  int schema = 0;
  if (!find_int_field(json, "schema_version", &schema) || !mqtt_ha_json_config_schema_valid(schema)) {
    out.error = "schema_version must be 2";
    return out;
  }
  int threshold = -1;
  if (find_int_field(json, "threshold_w", &threshold)) {
    if (threshold < 0 || threshold > 50000) {
      out.error = "threshold_w out of range";
      return out;
    }
    out.threshold_w = threshold;
  }
  int hour_end = -1;
  if (find_int_field(json, "hour_end", &hour_end)) {
    if (hour_end < 0 || hour_end > 2400) {
      out.error = "hour_end out of range";
      return out;
    }
    out.hour_end = hour_end;
  }
  int pmin = -1;
  if (find_int_field(json, "power_min_w", &pmin)) {
    if (pmin < -50000 || pmin > 50000) {
      out.error = "power_min_w out of range";
      return out;
    }
    out.power_min_w = pmin;
  }
  int pmax = -1;
  if (find_int_field(json, "power_max_w", &pmax)) {
    if (pmax < -50000 || pmax > 50000) {
      out.error = "power_max_w out of range";
      return out;
    }
    out.power_max_w = pmax;
  }
  std::string mode;
  if (find_string_field(json, "mode", &mode)) {
    const int ty = mode_string_to_type(mode);
    if (ty == 0) {
      out.error = "mode must be off, on, or power";
      return out;
    }
    out.mode_type = ty;
  }
  out.ok = out.threshold_w >= 0 || out.hour_end >= 0 || out.power_min_w >= 0 || out.power_max_w >= 0 ||
           out.mode_type > 0;
  if (!out.ok) out.error = "no recognized fields";
  return out;
}

MqttSiteConfigPatch mqtt_ha_json_config_parse_site(const std::string &json) {
  MqttSiteConfigPatch out;
  if (!object_root(json)) {
    out.error = "expected JSON object";
    return out;
  }
  int schema = 0;
  if (!find_int_field(json, "schema_version", &schema) || !mqtt_ha_json_config_schema_valid(schema)) {
    out.error = "schema_version must be 2";
    return out;
  }
  int max_w = -1;
  if (find_int_field(json, "max_routed_w", &max_w)) {
    if (max_w < 0 || max_w > 20000) {
      out.error = "max_routed_w out of range";
      return out;
    }
    out.max_routed_w = max_w;
  }
  bool stale_off = false;
  if (find_bool_field(json, "triac_off_when_source_stale", &stale_off)) {
    out.has_triac_off_when_source_stale = true;
    out.triac_off_when_source_stale = stale_off;
  }
  bool heater_idle = false;
  if (find_bool_field(json, "triac_backoff_when_heater_idle", &heater_idle)) {
    out.has_triac_backoff_when_heater_idle = true;
    out.triac_backoff_when_heater_idle = heater_idle;
  }
  int cap_idx = -1;
  if (find_int_field(json, "action_index", &cap_idx)) {
    uint32_t cap_wh = 0;
    int cap_v = -1;
    if (!find_int_field(json, "daily_cap_wh", &cap_v) || cap_v < 0) {
      out.error = "daily_cap_wh required with action_index";
      return out;
    }
    if (cap_idx < 0 || cap_idx >= 20) {
      out.error = "action_index out of range";
      return out;
    }
    out.action_daily_cap_index = cap_idx;
    out.action_daily_cap_wh = static_cast<uint32_t>(cap_v);
  }
  out.ok = out.max_routed_w >= 0 || out.has_triac_off_when_source_stale ||
           out.has_triac_backoff_when_heater_idle || out.action_daily_cap_index >= 0;
  if (!out.ok) out.error = "no recognized fields";
  return out;
}

MqttVacationConfigPatch mqtt_ha_json_config_parse_vacation(const std::string &json) {
  MqttVacationConfigPatch out;
  if (!object_root(json)) {
    out.error = "expected JSON object";
    return out;
  }
  int schema = 0;
  if (!find_int_field(json, "schema_version", &schema) || !mqtt_ha_json_config_schema_valid(schema)) {
    out.error = "schema_version must be 2";
    return out;
  }
  bool enabled = false;
  if (find_bool_field(json, "vacation_enabled", &enabled)) {
    out.has_vacation_enabled = true;
    out.vacation_enabled = enabled;
  }
  int end_epoch = -1;
  if (find_int_field(json, "vacation_end_epoch", &end_epoch)) {
    if (end_epoch < 0) {
      out.error = "vacation_end_epoch invalid";
      return out;
    }
    out.vacation_end_epoch = end_epoch;
  }
  out.ok = out.has_vacation_enabled || out.vacation_end_epoch >= 0;
  if (!out.ok) out.error = "no recognized fields";
  return out;
}
