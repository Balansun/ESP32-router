#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_SHELLY_EM

#include "shelly_em_logic.h"

#include "json_field_parse.h"

#include <cmath>
#include <cstring>

bool shelly_em_logic_parse_monophase_json(const char *json, ShellyEmMonoReading &out) {
  if (!json) return false;
  String body(json);
  if (body.indexOf("true") < 0) return false;
  body = body + ",";
  const float pw = parse_json_float("power", body);
  out.power_w = pw;
  out.voltage_v = parse_json_float("voltage", body);
  out.pf = std::fabs(parse_json_float("pf", body));
  out.energy_import_wh = static_cast<long>(parse_json_long("total\"", body));
  out.energy_export_wh = static_cast<long>(parse_json_long("total_returned", body));
  if (pw >= 0) {
    out.active_import_w = static_cast<int>(pw);
    out.active_export_w = 0;
    if (out.pf > 0) out.apparent_import_va = static_cast<int>(pw / out.pf);
    out.apparent_export_va = 0;
  } else {
    out.active_import_w = 0;
    out.active_export_w = static_cast<int>(-pw);
    if (out.pf > 0) out.apparent_export_va = static_cast<int>(-pw / out.pf);
    out.apparent_import_va = 0;
  }
  return true;
}

#endif
