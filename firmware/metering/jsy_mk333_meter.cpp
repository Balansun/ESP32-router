/*
 * jsy_mk333_meter.cpp — Source JsyMk333: JSY-MK-333 Modbus on Serial2 (JsyMk333SerialBaud, default 9600).
 * See: /en/build/pinout/sources/jsy-mk333 source_jsy_mk333; GUIDE A.5.2.
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_JSY_MK333
#include "balansun_globals.h"
// JSY-MK-333 (three-phase) Modbus subset.

void jsy_mk333_setup(void) {
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

void jsy_mk333_poll(void) {
  jsy_mk333_send_request();
  delay(8);
  byte buf[200];
  int a = 0;
  while (Serial2.available() && a < 200) {
    buf[a++] = (byte)Serial2.read();
  }
  if (a != 141) {
    MK333_rawData = "<strong>JSY-MK-333</strong><br>Trame " + String(a) + " octets (attendu 141)";
    return;
  }

  float T1 = ((buf[3] * 256) + buf[4]) / 100.0f;
  float T2 = ((buf[5] * 256) + buf[6]) / 100.0f;
  float T3 = ((buf[7] * 256) + buf[8]) / 100.0f;
  float I1 = ((buf[9] * 256) + buf[10]) / 100.0f;
  float I2 = ((buf[11] * 256) + buf[12]) / 100.0f;
  float I3 = ((buf[13] * 256) + buf[14]) / 100.0f;
  bool s1 = (buf[104] & 0x01) != 0;
  bool s2 = ((buf[104] >> 1) & 0x01) != 0;
  bool s3 = ((buf[104] >> 2) & 0x01) != 0;
  if (s1) I1 *= -1;
  if (s2) I2 *= -1;
  if (s3) I3 *= -1;
  bool injection = ((buf[104] >> 3) & 0x01) != 0;

  float Ptot = (float)(((uint32_t)buf[21] << 24) | ((uint32_t)buf[22] << 16) | ((uint32_t)buf[23] << 8) | (uint32_t)buf[24]);
  float pva1 = (float)((buf[35] * 256) + buf[36]);
  float pva2 = (float)((buf[37] * 256) + buf[38]);
  float pva3 = (float)((buf[39] * 256) + buf[40]);
  if (s1) pva1 = -pva1;
  if (s2) pva2 = -pva2;
  if (s3) pva3 = -pva3;
  float pvaSum = pva1 + pva2 + pva3;

  int32_t ws = ((int32_t)buf[119] << 24) | ((uint32_t)buf[120] << 16) | ((uint32_t)buf[121] << 8) | (uint32_t)buf[122];
  int32_t wi = ((int32_t)buf[135] << 24) | ((uint32_t)buf[136] << 16) | ((uint32_t)buf[137] << 8) | (uint32_t)buf[138];
  float JSY_Sout = (float)ws * 10.0f;
  float JSY_Inj = (float)wi * 10.0f;
  house_energy_import_wh = (long)(JSY_Sout / 10.0f);
  house_energy_export_wh = (long)(JSY_Inj / 10.0f);

  if (injection) {
    house_active_import_w = 0;
    house_active_export_w = (int)Ptot;
    house_apparent_import_va = 0;
    house_apparent_export_va = (int)fabsf(pvaSum);
  } else {
    house_active_import_w = (int)Ptot;
    house_active_export_w = 0;
    house_apparent_import_va = (int)fabsf(pvaSum);
    house_apparent_export_va = 0;
  }
  house_voltage_v = (T1 + T2 + T3) / 3.0f;
  house_current_a = (fabsf(I1) + fabsf(I2) + fabsf(I3)) / 3.0f;
  house_power_factor = (house_apparent_import_va > 0) ? (float)house_active_import_w / (float)house_apparent_import_va : 1.0f;

  MK333_rawData = "<strong>JSY-MK-333</strong><br>U1=" + String(T1) + "V I1=" + String(I1) + "A<br>";
  MK333_rawData += "Pw=" + String((int)Ptot) + "W inj=" + String(injection ? "oui" : "non");

  meter_reading_valid = true;
  esp_task_wdt_reset();
  if (cptLEDyellow > 30) {
    cptLEDyellow = 4;
  }
}

#endif /* BALANSUN_ENABLE_SOURCE_JSY_MK333 */
