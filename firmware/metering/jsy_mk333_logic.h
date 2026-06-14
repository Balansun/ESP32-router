#pragma once

/* jsy_mk333_logic.h — Modbus RTU parse for JSY-MK-333 triphase block @ 0x0100. Host-testable. */

#include <cstdint>

struct JsyMk333Reading {
  float tension_avg_v = 0;
  float intensite_avg_a = 0;
  int house_active_import_w = 0;
  int house_active_export_w = 0;
  int house_apparent_import_va = 0;
  int house_apparent_export_va = 0;
  bool injection = false;
};

/** Parse 141-byte Modbus RTU response (function 0x03 @ 0x0100). */
bool jsy_mk333_parse_modbus_frame(const uint8_t *bytes, int len, JsyMk333Reading &out);
