#include "balansun_meter_transport_guards.h"

#if BALANSUN_METER_NEEDS_JSON_FLAT

#include "json_flat_meter_logic.h"

#include "json_field_parse.h"

#include <cmath>
#include <cstring>

float json_flat_meter_logic_float_field(const std::string &json, const char *key) {
  String body(json.c_str());
  String nom = String("\"") + key + "\"";
  return parse_json_float(nom, body);
}

bool json_flat_meter_logic_parse_smartg(const std::string &json, JsonFlatMeterReading &out) {
  out.active_import_w = static_cast<int>(json_flat_meter_logic_float_field(json, "PowerDelivered_total"));
  out.active_export_w = static_cast<int>(json_flat_meter_logic_float_field(json, "PowerReturned_total"));
  out.energy_import_wh =
      static_cast<long>(1000.0f * json_flat_meter_logic_float_field(json, "EnergyDeliveredTariff1") +
                        1000.0f * json_flat_meter_logic_float_field(json, "EnergyDeliveredTariff2"));
  out.energy_export_wh =
      static_cast<long>(1000.0f * json_flat_meter_logic_float_field(json, "EnergyReturnedTariff1") +
                        1000.0f * json_flat_meter_logic_float_field(json, "EnergyReturnedTariff2"));
  return out.active_import_w > 0 || out.active_export_w > 0;
}

bool json_flat_meter_logic_parse_homewizard(const std::string &json, JsonFlatMeterReading &out) {
  const float ap = json_flat_meter_logic_float_field(json, "active_power_w");
  if (ap > 0) {
    out.active_import_w = static_cast<int>(ap);
    out.active_export_w = 0;
  } else {
    out.active_import_w = 0;
    out.active_export_w = static_cast<int>(-ap);
  }
  out.energy_import_wh =
      static_cast<long>(1000.0f * json_flat_meter_logic_float_field(json, "total_power_import_t1_kwh") +
                        1000.0f * json_flat_meter_logic_float_field(json, "total_power_import_t2_kwh"));
  out.energy_export_wh =
      static_cast<long>(1000.0f * json_flat_meter_logic_float_field(json, "total_power_export_t1_kwh") +
                        1000.0f * json_flat_meter_logic_float_field(json, "total_power_export_t2_kwh"));
  return ap != 0.0f || out.energy_import_wh > 0;
}

bool json_flat_meter_logic_parse_enphase_net(const std::string &net_conso_json, JsonFlatMeterReading &out) {
  const float pact = json_flat_meter_logic_float_field(net_conso_json, "actPower");
  const float pva = json_flat_meter_logic_float_field(net_conso_json, "apprntPwr");
  if (pact < 0) {
    out.active_import_w = 0;
    out.active_export_w = static_cast<int>(-pact);
  } else {
    out.active_export_w = 0;
    out.active_import_w = static_cast<int>(pact);
  }
  if (pva < 0) {
    out.apparent_import_va = 0;
    out.apparent_export_va = static_cast<int>(-pva);
  } else {
    out.apparent_export_va = 0;
    out.apparent_import_va = static_cast<int>(pva);
  }
  return pact != 0.0f || pva != 0.0f;
}

#endif
