#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_SHELLY_PRO
#include "shelly_pro_logic.h"

#include "balansun_globals.h"
#include "json_field_parse.h"

bool shelly_pro_logic_apply_three_phase_status(const String &shelly_data, const String &device_name,
                                               int voie, String &raw_out) {
  if (device_name.indexOf("shellypro3em") != 0 || voie != 3) {
    return false;
  }
  String tmp = prefilter_json("em:0", ":", shelly_data);
  float pw = parse_json_float("total_act_power", tmp);
  float t1 = parse_json_float("a_voltage", tmp);
  float t2 = parse_json_float("b_voltage", tmp);
  float t3 = parse_json_float("c_voltage", tmp);
  float pf1 = fabsf(parse_json_float("a_pf", tmp));
  float pf2 = fabsf(parse_json_float("b_pf", tmp));
  float pf3 = fabsf(parse_json_float("c_pf", tmp));
  float voltage = (t1 + t2 + t3) / 3.0f;
  float pf = abs((pf1 + pf2 + pf3) / 3.0f);
  if (pf > 1.0f) {
    pf = 1.0f;
  }
  if (pw >= 0) {
    house_active_import_w = (int)pw;
    house_active_export_w = 0;
    house_apparent_import_va = (pf > 0.01f) ? (int)(pw / pf) : 0;
    house_apparent_export_va = 0;
  } else {
    house_active_import_w = 0;
    house_active_export_w = (int)(-pw);
    house_apparent_export_va = (pf > 0.01f) ? (int)((-pw) / pf) : 0;
    house_apparent_import_va = 0;
  }
  tmp = prefilter_json("emdata:0", ":", shelly_data);
  house_energy_import_wh = (long)parse_json_float("total_act", tmp);
  house_energy_export_wh = (long)parse_json_float("total_act_ret", tmp);
  house_power_factor = pf;
  house_voltage_v = voltage;
  raw_out = "<strong>" + device_name + "</strong><br>" + shelly_data;
  return true;
}

#endif /* BALANSUN_ENABLE_SOURCE_SHELLY_PRO */
