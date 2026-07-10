#include "victron_gx_logic.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {

bool json_value_is_null(const std::string &json) {
  const char *k = "\"value\"";
  const size_t pos = json.find(k);
  if (pos == std::string::npos) return false;
  size_t i = pos + strlen(k);
  while (i < json.size() && (json[i] == ' ' || json[i] == ':' || json[i] == '\t')) i++;
  return i + 4 <= json.size() && json.compare(i, 4, "null") == 0;
}

}  // namespace

bool balansun_victron_parse_value_json(const std::string &json, float &out) {
  if (json_value_is_null(json)) return false;
  const char *k = "\"value\"";
  const size_t pos = json.find(k);
  if (pos == std::string::npos) return false;
  size_t i = pos + strlen(k);
  while (i < json.size() && (json[i] == ' ' || json[i] == ':' || json[i] == '\t')) i++;
  if (i >= json.size()) return false;
  char *end = nullptr;
  const float v = strtof(json.c_str() + i, &end);
  if (end == json.c_str() + i) return false;
  out = v;
  return std::isfinite(v);
}

VictronHousePower balansun_victron_map_surplus(const VictronRawInputs &in, VictronSurplusMode mode) {
  VictronHousePower hp;
  const int battery_charge =
      (in.have_battery && in.battery_w > 0.0f) ? static_cast<int>(in.battery_w) : 0;
  const int battery_discharge =
      (in.have_battery && in.battery_w < 0.0f) ? static_cast<int>(-in.battery_w) : 0;
  const int grid_export = (in.have_grid && in.grid_w < 0.0f) ? static_cast<int>(-in.grid_w) : 0;
  const int grid_import = (in.have_grid && in.grid_w > 0.0f) ? static_cast<int>(in.grid_w) : 0;

  switch (mode) {
    case VictronSurplusMode::BatteryCharge:
      hp.active_export_w = battery_charge;
      hp.active_import_w = battery_discharge;
      break;
    case VictronSurplusMode::GridExport:
      hp.active_export_w = grid_export;
      hp.active_import_w = grid_import;
      break;
    case VictronSurplusMode::CombinedMax:
      hp.active_export_w = std::max(battery_charge, grid_export);
      hp.active_import_w = std::max(battery_discharge, grid_import);
      break;
  }
  return hp;
}

const char *balansun_victron_surplus_mode_wire(VictronSurplusMode mode) {
  switch (mode) {
    case VictronSurplusMode::BatteryCharge:
      return "battery_charge";
    case VictronSurplusMode::GridExport:
      return "grid_export";
    case VictronSurplusMode::CombinedMax:
      return "combined_max";
  }
  return "battery_charge";
}

bool balansun_victron_surplus_mode_from_wire(const char *wire, VictronSurplusMode &out) {
  if (!wire || !wire[0]) return false;
  if (strcmp(wire, "battery_charge") == 0) {
    out = VictronSurplusMode::BatteryCharge;
    return true;
  }
  if (strcmp(wire, "grid_export") == 0) {
    out = VictronSurplusMode::GridExport;
    return true;
  }
  if (strcmp(wire, "combined_max") == 0) {
    out = VictronSurplusMode::CombinedMax;
    return true;
  }
  return false;
}
