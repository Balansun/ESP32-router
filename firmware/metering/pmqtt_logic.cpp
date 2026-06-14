#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_PMqtt

#include "pmqtt_logic.h"

#include "json_field_parse.h"

#include <cmath>

namespace {

bool schema_has_token(const std::string &schema, const char *token) {
  const std::string t(token);
  size_t pos = 0;
  while ((pos = schema.find(t, pos)) != std::string::npos) {
    const bool left = pos == 0 || schema[pos - 1] == ',';
    const bool right = pos + t.size() == schema.size() || schema[pos + t.size()] == ',';
    if (left && right) return true;
    pos += 1;
  }
  return false;
}

}  // namespace

PmqttPayloadKind pmqtt_logic_classify_payload(const std::string &json) {
  if (json.find("\"house\"") != std::string::npos) {
    return PmqttPayloadKind::HouseSnapshot;
  }
  if (json.find("\"Pw\"") != std::string::npos || json.find("\"power_w\"") != std::string::npos ||
      json.find("\"active_power_w\"") != std::string::npos) {
    return PmqttPayloadKind::PwPf;
  }
  return PmqttPayloadKind::None;
}

bool pmqtt_logic_parse_pw_schema(const std::string &json, const std::string &schema, PmqttPwReading &out) {
  String body(json.c_str());
  float pw = 0;
  bool have = false;
  if (schema_has_token(schema, "active_power_w")) {
    pw = parse_json_float("\"active_power_w\"", body);
    if (pw != 0) have = true;
  }
  if (!have && schema_has_token(schema, "power_w")) {
    pw = parse_json_float("\"power_w\"", body);
    if (pw != 0) have = true;
  }
  if (!have && schema_has_token(schema, "Pw")) {
    pw = parse_json_float("\"Pw\"", body);
    have = true;
  }
  if (!have) return false;

  out.pf = 1.0f;
  if (schema.find("Pf") != std::string::npos) {
    out.pf = std::fabs(parse_json_float("\"Pf\"", body));
    if (out.pf > 1.0f) out.pf = 1.0f;
  }
  if (pw >= 0) {
    out.house_active_import_w = static_cast<int>(pw);
    out.house_active_export_w = 0;
    out.house_apparent_import_va = out.pf > 0.01f ? static_cast<int>(pw / out.pf) : out.house_active_import_w;
    out.house_apparent_export_va = 0;
  } else {
    out.house_active_import_w = 0;
    out.house_active_export_w = static_cast<int>(-pw);
    out.house_apparent_export_va = out.pf > 0.01f ? static_cast<int>((-pw) / out.pf) : out.house_active_export_w;
    out.house_apparent_import_va = 0;
  }
  return true;
}

#endif
