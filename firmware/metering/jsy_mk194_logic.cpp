#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_JSY_MK194

#include "jsy_mk194_logic.h"

bool jsy_mk194_parse_modbus_frame(const uint8_t *bytes, int len, JsyMk194Reading &out) {
  if (!bytes || len != 61) return false;
  long les_datas[14] = {0};
  int j = 3;
  for (int i = 0; i < 14; i++) {
    les_datas[i] = 0;
    les_datas[i] += static_cast<long>(bytes[j]) << 24;
    j += 1;
    les_datas[i] += static_cast<long>(bytes[j]) << 16;
    j += 1;
    les_datas[i] += static_cast<long>(bytes[j]) << 8;
    j += 1;
    les_datas[i] += static_cast<long>(bytes[j]);
    j += 1;
  }
  out.sens_1 = bytes[27];
  out.sens_2 = bytes[28];
  out.voltage_second_v = static_cast<float>(les_datas[0]) * 0.0001f;
  out.current_second_a = static_cast<float>(les_datas[1]) * 0.0001f;
  out.power_second_w = static_cast<int>(les_datas[2] * 0.0001);
  out.energy_import_wh = static_cast<long>(les_datas[3] * 0.1);
  out.pf_second = static_cast<float>(les_datas[4]) * 0.001f;
  out.energy_export_wh = static_cast<long>(les_datas[5] * 0.1);
  out.frequence_hz = static_cast<float>(les_datas[7]) * 0.01f;
  int pva1 = 0;
  if (out.pf_second > 0) {
    pva1 = static_cast<int>(static_cast<float>(out.power_second_w) / out.pf_second);
  }
  if (out.sens_1 > 0) {
    out.second_active_export_w = out.power_second_w;
    out.second_active_import_w = 0;
    out.pva_t = pva1;
  } else {
    out.second_active_import_w = out.power_second_w;
    out.second_active_export_w = 0;
    out.pva_t = pva1;
  }
  out.voltage_house_v = static_cast<float>(les_datas[8]) * 0.0001f;
  out.current_house_a = static_cast<float>(les_datas[9]) * 0.0001f;
  out.power_house_w = static_cast<int>(les_datas[10] * 0.0001);
  out.house_energy_import_wh_wh = static_cast<long>(les_datas[11] * 0.1);
  out.pf_house = static_cast<float>(les_datas[12]) * 0.001f;
  out.house_energy_export_wh_wh = static_cast<long>(les_datas[13] * 0.1);
  int pva2 = 0;
  if (out.pf_house > 0) {
    pva2 = static_cast<int>(static_cast<float>(out.power_house_w) / out.pf_house);
  }
  if (out.sens_2 > 0) {
    out.house_active_export_w = out.power_house_w;
    out.house_active_import_w = 0;
    out.pva_m = pva2;
  } else {
    out.house_active_import_w = out.power_house_w;
    out.house_active_export_w = 0;
    out.pva_m = pva2;
  }
  return true;
}

#endif
