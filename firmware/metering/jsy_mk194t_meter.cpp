/*
 * jsy_mk194t_meter.cpp — Source JsyMk194: JSY-MK-194T Modbus on Serial2 @ 4800, reg block 0x0048.
 * See: /en/build/pinout/sources/jsy-mk194 source_jsy_mk194; GUIDE A.5.1.
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_JSY_MK194
#include "jsy_mk194t_meter.h"

#include "balansun_globals.h"
#include "balansun_hw_presence.h"
#include "jsy_mk194_logic.h"

#include <esp_task_wdt.h>

static void appendUxiWaveformDiagnostics(JsonObject doc) {
  JsonObject wf = doc["uxi_waveform"].to<JsonObject>();
  JsonArray vArr = wf["volt_m"].to<JsonArray>();
  JsonArray aArr = wf["amp_m"].to<JsonArray>();
  int i0 = 0;
  for (int i = 0; i < 100; i++) {
    const int i1 = (i + 1) % 100;
    if (voltM[i] <= 0 && voltM[i1] > 0) {
      i0 = i1;
      break;
    }
  }
  for (int i = 0; i < 100; i++) {
    const int j = (i + i0) % 100;
    vArr.add(voltM[j]);
    aArr.add(ampM[j]);
  }
}

void JsyMk194Meter::setup() {
  /* UART on first poll (core 0): Serial2.begin from setup() can trip INT WDT on ESP32-S3. */
}

static bool jsy_mk194t_uart_ready(void) {
  static bool ready = false;
  if (ready) {
    return true;
  }
#if CONFIG_IDF_TARGET_ESP32S3
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

static void apply_jsy_reading(const JsyMk194Reading &rd, bool apply_house) {
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
  if (apply_house) {
    house_voltage_v = rd.voltage_house_v;
    house_current_a = rd.current_house_a;
    house_energy_import_wh = rd.house_energy_import_wh_wh;
    house_power_factor = rd.pf_house;
    house_energy_export_wh = rd.house_energy_export_wh_wh;
    house_active_import_w = rd.house_active_import_w;
    house_active_export_w = rd.house_active_export_w;
    house_apparent_import_va = rd.pva_m;
    house_apparent_export_va = (rd.sens_2 > 0) ? rd.pva_m : 0;
  }
  meter_reading_valid = true;
  esp_task_wdt_reset();
  if (cptLEDyellow > 30) {
    cptLEDyellow = 4;
  }
}

static bool jsy_mk194t_poll_modbus(bool apply_house) {
  if (millis() < 5000UL) {
    balansun_hw_presence_on_jsy_poll(false, false);
    return false;
  }
  const bool uart_ok = jsy_mk194t_uart_ready();
  if (!uart_ok) {
    balansun_hw_presence_on_jsy_poll(false, false);
    return false;
  }
  const byte msg_send[] = {0x01, 0x03, 0x00, 0x48, 0x00, 0x0E, 0x44, 0x18};
  for (int i = 0; i < 8; i++) {
    Serial2.write(msg_send[i]);
  }

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
      apply_jsy_reading(rd, apply_house);
    }
  }
  balansun_hw_presence_on_jsy_poll(true, frame_ok);
  if (frame_ok) {
    g_second_channel_meter_valid_this_boot = true;
  }
  return frame_ok;
}

bool JsyMk194Meter::pollTransport() { return jsy_mk194t_poll_modbus(true); }

bool JsyMk194Meter::pollTriacChannelOnly() { return jsy_mk194t_poll_modbus(false); }

void JsyMk194Meter::appendDiagnostics(JsonObject doc, int linky_tail_max) {
  (void)linky_tail_max;
  appendUxiWaveformDiagnostics(doc);
}

IMeterDriver *balansun_meter_instance_jsy_mk194() {
  static JsyMk194Meter instance;
  return &instance;
}

void jsy_mk194t_setup(void) { balansun_meter_instance_jsy_mk194()->setup(); }

void jsy_mk194t_poll(void) { balansun_meter_instance_jsy_mk194()->poll(); }

bool jsy_mk194t_poll_triac_channel(void) {
  return static_cast<JsyMk194Meter *>(balansun_meter_instance_jsy_mk194())->pollTriacChannelOnly();
}

#endif /* BALANSUN_ENABLE_SOURCE_JSY_MK194 */
