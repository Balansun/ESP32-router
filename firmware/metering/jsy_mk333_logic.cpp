#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_JSY_MK333

#include "jsy_mk333_logic.h"

#include <cmath>

bool jsy_mk333_parse_modbus_frame(const uint8_t *bytes, int len, JsyMk333Reading &out) {
  if (!bytes || len != 141) return false;
  const float t1 = static_cast<float>((bytes[3] << 8) + bytes[4]) / 100.0f;
  const float t2 = static_cast<float>((bytes[5] << 8) + bytes[6]) / 100.0f;
  const float t3 = static_cast<float>((bytes[7] << 8) + bytes[8]) / 100.0f;
  float i1 = static_cast<float>((bytes[9] << 8) + bytes[10]) / 100.0f;
  float i2 = static_cast<float>((bytes[11] << 8) + bytes[12]) / 100.0f;
  float i3 = static_cast<float>((bytes[13] << 8) + bytes[14]) / 100.0f;
  const bool s1 = (bytes[104] & 0x01) != 0;
  const bool s2 = ((bytes[104] >> 1) & 0x01) != 0;
  const bool s3 = ((bytes[104] >> 2) & 0x01) != 0;
  out.injection = ((bytes[104] >> 3) & 0x01) != 0;
  if (s1) i1 *= -1;
  if (s2) i2 *= -1;
  if (s3) i3 *= -1;
  const float ptot = static_cast<float>(((uint32_t)bytes[21] << 24) | ((uint32_t)bytes[22] << 16) |
                                        ((uint32_t)bytes[23] << 8) | (uint32_t)bytes[24]);
  float pva1 = static_cast<float>((bytes[35] << 8) + bytes[36]);
  float pva2 = static_cast<float>((bytes[37] << 8) + bytes[38]);
  float pva3 = static_cast<float>((bytes[39] << 8) + bytes[40]);
  if (s1) pva1 = -pva1;
  if (s2) pva2 = -pva2;
  if (s3) pva3 = -pva3;
  const float pva_sum = pva1 + pva2 + pva3;
  out.tension_avg_v = (t1 + t2 + t3) / 3.0f;
  out.intensite_avg_a = (std::fabs(i1) + std::fabs(i2) + std::fabs(i3)) / 3.0f;
  if (out.injection) {
    out.house_active_import_w = 0;
    out.house_active_export_w = static_cast<int>(ptot);
    out.house_apparent_import_va = 0;
    out.house_apparent_export_va = static_cast<int>(std::fabs(pva_sum));
  } else {
    out.house_active_import_w = static_cast<int>(ptot);
    out.house_active_export_w = 0;
    out.house_apparent_import_va = static_cast<int>(std::fabs(pva_sum));
    out.house_apparent_export_va = 0;
  }
  return true;
}

#endif
