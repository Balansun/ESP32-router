/*
 * external_peer_logic.cpp — measurements JSON parser for Source Ext.
 * Sign convention matches local sources (GUIDE A.2). See: external_peer_logic.h.
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_EXT

#include "external_peer_logic.h"

#include <cctype>
#include <cstdlib>

namespace {

bool json_int_after(const std::string &body, const char *key, int &out) {
  const std::string needle = std::string("\"") + key + "\":";
  const auto pos = body.find(needle);
  if (pos == std::string::npos) return false;
  size_t i = pos + needle.size();
  while (i < body.size() && std::isspace(static_cast<unsigned char>(body[i]))) i++;
  if (i >= body.size()) return false;
  char *end = nullptr;
  const long raw = std::strtol(body.c_str() + i, &end, 10);
  if (end == body.c_str() + i) return false;
  out = static_cast<int>(raw);
  return true;
}

bool json_float_after(const std::string &body, const char *key, float &out) {
  const std::string needle = std::string("\"") + key + "\":";
  const auto pos = body.find(needle);
  if (pos == std::string::npos) return false;
  size_t i = pos + needle.size();
  while (i < body.size() && std::isspace(static_cast<unsigned char>(body[i]))) i++;
  if (i >= body.size()) return false;
  char *end = nullptr;
  const float raw = std::strtof(body.c_str() + i, &end);
  if (end == body.c_str() + i) return false;
  out = raw;
  return true;
}

size_t find_house_block(const std::string &body) { return body.find("\"house\""); }

}  // namespace

bool external_peer_logic_parse_measurements_json(const std::string &body, ExternalPeerReading &out) {
  out = ExternalPeerReading{};
  const size_t house_pos = find_house_block(body);
  if (house_pos == std::string::npos) return false;
  const std::string slice = body.substr(house_pos);
  if (!json_int_after(slice, "active_import_w", out.house_active_import_w)) return false;
  if (!json_int_after(slice, "active_export_w", out.house_active_export_w)) {
    out.house_active_export_w = 0;
  }
  json_int_after(slice, "apparent_import_va", out.house_apparent_import_va);
  json_int_after(slice, "apparent_export_va", out.house_apparent_export_va);
  float ej = 0;
  if (json_float_after(slice, "energy_day_import_wh", ej)) {
    out.house_day_energy_import_wh = ej;
  }
  if (json_float_after(slice, "energy_day_export_wh", ej)) {
    out.house_day_energy_export_wh = ej;
  }
  if (json_float_after(slice, "energy_total_import_wh", ej)) {
    out.house_energy_import_wh = ej;
  }
  if (json_float_after(slice, "energy_total_export_wh", ej)) {
    out.house_energy_export_wh = ej;
  }
  const auto second = body.find("\"second\"");
  if (second != std::string::npos) {
    const std::string sec = body.substr(second);
    json_int_after(sec, "active_import_w", out.second_active_import_w);
    json_int_after(sec, "active_export_w", out.second_active_export_w);
  }
  out.valid = true;
  return true;
}

#endif
