#pragma once

/* json_flat_meter_logic.h — Flat JSON parsers for SmartG and HomeWizard HTTP APIs. */

#include <string>

struct JsonFlatMeterReading {
  int active_import_w = 0;
  int active_export_w = 0;
  int apparent_import_va = 0;
  int apparent_export_va = 0;
  long energy_import_wh = 0;
  long energy_export_wh = 0;
};

/** Extract float after `"key":` in flat JSON fragment (SmartG / HomeW style). */
float json_flat_meter_logic_float_field(const std::string &json, const char *key);

/** Smart Gateways `/smartmeter/api/read` body (inner JSON object). */
bool json_flat_meter_logic_parse_smartg(const std::string &json, JsonFlatMeterReading &out);

/** HomeWizard `/api/v1/data` body (signed active_power_w). */
bool json_flat_meter_logic_parse_homewizard(const std::string &json, JsonFlatMeterReading &out);

/** Enphase Envoy net-consumption cumulative slice. */
bool json_flat_meter_logic_parse_enphase_net(const std::string &net_conso_json, JsonFlatMeterReading &out);
