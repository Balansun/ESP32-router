/*
 * jsy_mk194t_meter.cpp — Source JsyMk194: JSY-MK-194T Modbus on Serial2 @ 4800, reg block 0x0048.
 * See: /en/build/pinout/sources/jsy-mk194 source_jsy_mk194; GUIDE A.5.1.
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_JSY_MK194
#include "balansun_globals.h"
#include "balansun_hw_presence.h"
#include "jsy_mk194_logic.h"

#include <esp_task_wdt.h>

void jsy_mk194t_setup() {
  /* UART on first poll (core 0): Serial2.begin from setup() can trip INT WDT on ESP32-S3. */
}

static bool jsy_mk194t_uart_ready(void) {
  static bool ready = false;
  if (ready) {
    return true;
  }
#if CONFIG_IDF_TARGET_ESP32S3
  /* Serial2.begin() from either setup or metering still trips INT WDT on DevKit+OPI; poll paused. */
  (void)ready;
  return false;
#else
  esp_task_wdt_reset();
  Serial2.setTimeout(20);
  Serial2.begin(4800, SERIAL_8N1, g_pins.uart_rx, g_pins.uart_tx);
  esp_task_wdt_reset();
  ready = true;
  return true;
#endif
}

void jsy_mk194t_poll() {
  if (millis() < 5000UL) {
    balansun_hw_presence_on_jsy_poll(false, false);
    return;
  }
  const bool uart_ok = jsy_mk194t_uart_ready();
  if (!uart_ok) {
    balansun_hw_presence_on_jsy_poll(false, false);
    return;
  }
  int i;
  byte msg_send[] = { 0x01, 0x03, 0x00, 0x48, 0x00, 0x0E, 0x44, 0x18 };
  // Modbus RTU request on Serial2
  for (i = 0; i < 8; i++) {
    Serial2.write(msg_send[i]);
  }

  // Response to previous poll (4800 baud only)
  int a = 0;
  constexpr int kJsyMaxRxBytes = 64;
  while (Serial2.available() && a < kJsyMaxRxBytes) {
    ByteArray[a] = Serial2.read();
    a++;
  }
  while (Serial2.available()) {
    Serial2.read();
  }


  bool frame_ok = false;
  if (a == 61) {
    JsyMk194Reading rd;
    if (jsy_mk194_parse_modbus_frame(ByteArray, a, rd)) {
      frame_ok = true;
      Sens_1 = rd.sens_1;
      Sens_2 = rd.sens_2;
      second_voltage_v = rd.voltage_second_v;
      second_current_a = rd.current_second_a;
      second_energy_import_wh = rd.energy_import_wh;
      second_power_factor = rd.pf_second;
      second_energy_export_wh = rd.energy_export_wh;
      mains_frequency_hz = rd.frequence_hz;
      second_active_import_w = rd.second_active_import_w;
      second_active_export_w = rd.second_active_export_w;
      second_apparent_import_va = rd.pva_t;
      second_apparent_export_va = (rd.sens_1 > 0) ? rd.pva_t : 0;
      house_voltage_v = rd.voltage_house_v;
      house_current_a = rd.current_house_a;
      house_energy_import_wh = rd.house_energy_import_wh_wh;
      house_power_factor = rd.pf_house;
      house_energy_export_wh = rd.house_energy_export_wh_wh;
      house_active_import_w = rd.house_active_import_w;
      house_active_export_w = rd.house_active_export_w;
      house_apparent_import_va = rd.pva_m;
      house_apparent_export_va = (rd.sens_2 > 0) ? rd.pva_m : 0;
      meter_reading_valid = true;
      esp_task_wdt_reset();
      if (cptLEDyellow > 30) {
        cptLEDyellow = 4;
      }
    }
  }
  balansun_hw_presence_on_jsy_poll(true, frame_ok);
}
#endif /* BALANSUN_ENABLE_SOURCE_JSY_MK194 */
