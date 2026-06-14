/*
 * jsy_mk333_meter.cpp — Source JsyMk333: JSY-MK-333 Modbus on Serial2 (JsyMk333SerialBaud, default 9600).
 * See: /en/build/pinout/sources/jsy-mk333 source_jsy_mk333; GUIDE A.5.2.
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_JSY_MK333
#include "jsy_mk333_meter.h"

#include "balansun_globals.h"
#include "jsy_mk333_logic.h"

#include <esp_task_wdt.h>

void JsyMk333Meter::setup() {
  Serial2.setRxBufferSize(1024);
  uint32_t b = JsyMk333SerialBaud;
  if (b < 1200 || b > 115200) {
    b = 9600;
  }
  Serial2.begin(b, SERIAL_8N1, g_pins.uart_rx, g_pins.uart_tx);
}

void jsy_mk333_send_request(void) {
  const byte msg_send[] = {0x01, 0x03, 0x01, 0x00, 0x00, 0x44, 0x44, 0x05};
  for (int i = 0; i < 8; i++) {
    Serial2.write(msg_send[i]);
  }
}

bool JsyMk333Meter::pollTransport() {
  jsy_mk333_send_request();
  delay(8);
  byte buf[200];
  int a = 0;
  while (Serial2.available() && a < 200) {
    buf[a++] = (byte)Serial2.read();
  }
  if (a != 141) {
    MK333_rawData = "<strong>JSY-MK-333</strong><br>Trame " + String(a) + " octets (attendu 141)";
    return false;
  }

  JsyMk333Reading rd;
  if (!jsy_mk333_parse_modbus_frame(buf, a, rd)) {
    return false;
  }

  const float t1 = static_cast<float>((buf[3] << 8) + buf[4]) / 100.0f;
  const float t2 = static_cast<float>((buf[5] << 8) + buf[6]) / 100.0f;
  const float t3 = static_cast<float>((buf[7] << 8) + buf[8]) / 100.0f;
  float i1 = static_cast<float>((buf[9] << 8) + buf[10]) / 100.0f;
  float i2 = static_cast<float>((buf[11] << 8) + buf[12]) / 100.0f;
  float i3 = static_cast<float>((buf[13] << 8) + buf[14]) / 100.0f;
  const bool s1 = (buf[104] & 0x01) != 0;
  const bool s2 = ((buf[104] >> 1) & 0x01) != 0;
  const bool s3 = ((buf[104] >> 2) & 0x01) != 0;
  if (s1) {
    i1 *= -1;
  }
  if (s2) {
    i2 *= -1;
  }
  if (s3) {
    i3 *= -1;
  }

  const int32_t ws = ((int32_t)buf[119] << 24) | ((uint32_t)buf[120] << 16) | ((uint32_t)buf[121] << 8) |
                      (uint32_t)buf[122];
  const int32_t wi = ((int32_t)buf[135] << 24) | ((uint32_t)buf[136] << 16) | ((uint32_t)buf[137] << 8) |
                       (uint32_t)buf[138];
  const float jsy_sout = static_cast<float>(ws) * 10.0f;
  const float jsy_inj = static_cast<float>(wi) * 10.0f;
  house_energy_import_wh = static_cast<long>(jsy_sout / 10.0f);
  house_energy_export_wh = static_cast<long>(jsy_inj / 10.0f);

  house_active_import_w = rd.house_active_import_w;
  house_active_export_w = rd.house_active_export_w;
  house_apparent_import_va = rd.house_apparent_import_va;
  house_apparent_export_va = rd.house_apparent_export_va;
  house_voltage_v = rd.tension_avg_v;
  house_current_a = rd.intensite_avg_a;
  house_power_factor = (house_apparent_import_va > 0)
                           ? static_cast<float>(house_active_import_w) / static_cast<float>(house_apparent_import_va)
                           : 1.0f;

  MK333_rawData = "<strong>JSY-MK-333</strong><br>U1=" + String(t1) + "V I1=" + String(i1) + "A<br>";
  const float ptot = static_cast<float>(((uint32_t)buf[21] << 24) | ((uint32_t)buf[22] << 16) |
                                        ((uint32_t)buf[23] << 8) | (uint32_t)buf[24]);
  MK333_rawData += "Pw=" + String((int)ptot) + "W inj=" + String(rd.injection ? "oui" : "non");

  meter_reading_valid = true;
  esp_task_wdt_reset();
  if (cptLEDyellow > 30) {
    cptLEDyellow = 4;
  }
  return true;
}

void JsyMk333Meter::appendDiagnostics(JsonObject doc, int linky_tail_max) {
  (void)linky_tail_max;
  JsonObject j3 = doc["jsy333"].to<JsonObject>();
  j3["raw"] = MK333_rawData;
  j3["serial_baud"] = JsyMk333SerialBaud;
}

IMeterDriver *balansun_meter_instance_jsy_mk333() {
  static JsyMk333Meter instance;
  return &instance;
}

#endif /* BALANSUN_ENABLE_SOURCE_JSY_MK333 */
