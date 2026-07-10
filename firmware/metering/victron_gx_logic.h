#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

enum class VictronSurplusMode : uint8_t {
  BatteryCharge = 0,
  GridExport = 1,
  CombinedMax = 2,
};

struct VictronRawInputs {
  bool have_battery = false;
  float battery_w = 0.0f;
  bool have_grid = false;
  float grid_w = 0.0f;
};

struct VictronHousePower {
  int active_import_w = 0;
  int active_export_w = 0;
};

/** Parse Victron MQTT JSON payload `{"value": <float|null>}`. */
bool balansun_victron_parse_value_json(const std::string &json, float &out);

VictronHousePower balansun_victron_map_surplus(const VictronRawInputs &in, VictronSurplusMode mode);

const char *balansun_victron_surplus_mode_wire(VictronSurplusMode mode);
bool balansun_victron_surplus_mode_from_wire(const char *wire, VictronSurplusMode &out);
