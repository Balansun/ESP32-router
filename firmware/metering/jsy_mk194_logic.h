#pragma once

/* jsy_mk194_logic.h — Modbus RTU frame parse for JSY-MK-194T (JsyMk194). Host-testable. */

#include <cstdint>

struct JsyMk194Reading {
  float voltage_second_v = 0;
  float current_second_a = 0;
  int power_second_w = 0;
  long energy_import_wh = 0;
  float pf_second = 0;
  long energy_export_wh = 0;
  float frequence_hz = 50;
  int sens_1 = 0;
  int sens_2 = 0;
  int pva_t = 0;
  int second_active_import_w = 0;
  int second_active_export_w = 0;
  float voltage_house_v = 0;
  float current_house_a = 0;
  int power_house_w = 0;
  long house_energy_import_wh_wh = 0;
  float pf_house = 0;
  long house_energy_export_wh_wh = 0;
  int house_active_import_w = 0;
  int house_active_export_w = 0;
  int pva_m = 0;
};

/** Parse 61-byte Modbus RTU response (function 0x03, 14 regs @ 0x48). */
bool jsy_mk194_parse_modbus_frame(const uint8_t *bytes, int len, JsyMk194Reading &out);
