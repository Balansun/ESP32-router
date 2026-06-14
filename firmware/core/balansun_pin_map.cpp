#include "balansun_pin_map.h"

#include "balansun_board.h"
#include "balansun_pin_logic.h"
#include "balansun_temperature.h"
#include "balansun_temperature_logic.h"
#include "balansun_source.h"
#include "balansun_source_logic.h"
#include <Arduino.h>

#if CONFIG_IDF_TARGET_ESP32S3
static bool balansun_target_is_s3() { return true; }
#else
static bool balansun_target_is_s3() { return false; }
#endif
#include <ArduinoJson.h>

BalansunPinMap g_pins = balansun_board_default_pins();

int8_t pinTriacDim = static_cast<int8_t>(kDefaultTriacDimGpio);
int8_t pinZeroCross = static_cast<int8_t>(kDefaultZeroCrossGpio);
int8_t pinUartRx = static_cast<int8_t>(kDefaultRxd2);
int8_t pinUartTx = static_cast<int8_t>(kDefaultTxd2);
int8_t pinTempGpio = static_cast<int8_t>(kDefaultPinTemp);
int8_t pinAnalog0 = static_cast<int8_t>(kDefaultAnalogIn0);
int8_t pinAnalog1 = static_cast<int8_t>(kDefaultAnalogIn1);
int8_t pinAnalog2 = static_cast<int8_t>(kDefaultAnalogIn2);
bool hwPinsPersistPresent = false;
bool pinMapRebootPending = false;
bool pinMapFallbackActive = false;

BalansunPinMap balansun_board_default_pins() {
  BalansunPinMap m;
  m.triac_dim = static_cast<int8_t>(kDefaultTriacDimGpio);
  m.zero_cross = static_cast<int8_t>(kDefaultZeroCrossGpio);
  m.uart_rx = static_cast<int8_t>(kDefaultRxd2);
  m.uart_tx = static_cast<int8_t>(kDefaultTxd2);
  m.temp = static_cast<int8_t>(kDefaultPinTemp);
  m.analog0 = static_cast<int8_t>(kDefaultAnalogIn0);
  m.analog1 = static_cast<int8_t>(kDefaultAnalogIn1);
  m.analog2 = static_cast<int8_t>(kDefaultAnalogIn2);
  return m;
}

const char *balansun_board_pin_profile_id() {
#if CONFIG_IDF_TARGET_ESP32S3
  return "esp32s3";
#else
  return "wroom32";
#endif
}

static void balansun_pins_assign_stored(const BalansunPinMap &m) {
  pinTriacDim = m.triac_dim;
  pinZeroCross = m.zero_cross;
  pinUartRx = m.uart_rx;
  pinUartTx = m.uart_tx;
  pinTempGpio = m.temp;
  pinAnalog0 = m.analog0;
  pinAnalog1 = m.analog1;
  pinAnalog2 = m.analog2;
}

void balansun_pin_map_resolve_legacy_core_pins(BalansunPinMap &map) {
  const BalansunPinMap d = balansun_board_default_pins();
  if (map.triac_dim == kBalansunPinTempDisabled) map.triac_dim = d.triac_dim;
  if (map.zero_cross == kBalansunPinTempDisabled) map.zero_cross = d.zero_cross;
  if (map.uart_rx == kBalansunPinTempDisabled) map.uart_rx = d.uart_rx;
  if (map.uart_tx == kBalansunPinTempDisabled) map.uart_tx = d.uart_tx;
  if (map.analog0 == kBalansunPinTempDisabled) map.analog0 = d.analog0;
  if (map.analog1 == kBalansunPinTempDisabled) map.analog1 = d.analog1;
  if (map.analog2 == kBalansunPinTempDisabled) map.analog2 = d.analog2;
}

bool balansun_pin_map_equal(const BalansunPinMap &a, const BalansunPinMap &b) {
  return a.triac_dim == b.triac_dim && a.zero_cross == b.zero_cross && a.uart_rx == b.uart_rx &&
         a.uart_tx == b.uart_tx && a.temp == b.temp && a.analog0 == b.analog0 && a.analog1 == b.analog1 &&
         a.analog2 == b.analog2;
}

BalansunPinMap balansun_pin_map_from_stored() {
  BalansunPinMap m;
  m.triac_dim = pinTriacDim;
  m.zero_cross = pinZeroCross;
  m.uart_rx = pinUartRx;
  m.uart_tx = pinUartTx;
  m.temp = pinTempGpio;
  m.analog0 = pinAnalog0;
  m.analog1 = pinAnalog1;
  m.analog2 = pinAnalog2;
  balansun_pin_map_resolve_legacy_core_pins(m);
  return m;
}

void balansun_pins_migrate_eeprom_load() {
  const int8_t raw[] = {pinTriacDim, pinZeroCross, pinUartRx, pinUartTx,
                        pinTempGpio, pinAnalog0, pinAnalog1, pinAnalog2};
  bool all_legacy = true;
  for (int8_t v : raw) {
    if (v != kBalansunPinTempDisabled) {
      all_legacy = false;
      break;
    }
  }
  if (all_legacy) {
    balansun_pins_assign_stored(balansun_board_default_pins());
    return;
  }
  BalansunPinMap m;
  m.triac_dim = pinTriacDim;
  m.zero_cross = pinZeroCross;
  m.uart_rx = pinUartRx;
  m.uart_tx = pinUartTx;
  m.temp = pinTempGpio;
  m.analog0 = pinAnalog0;
  m.analog1 = pinAnalog1;
  m.analog2 = pinAnalog2;
  balansun_pin_map_resolve_legacy_core_pins(m);
  pinTriacDim = m.triac_dim;
  pinZeroCross = m.zero_cross;
  pinUartRx = m.uart_rx;
  pinUartTx = m.uart_tx;
  pinAnalog0 = m.analog0;
  pinAnalog1 = m.analog1;
  pinAnalog2 = m.analog2;
}

void balansun_pins_early_safe_defaults() {
  const BalansunPinMap d = balansun_board_default_pins();
  pinMode(d.zero_cross, INPUT_PULLDOWN);
  pinMode(d.triac_dim, OUTPUT);
  digitalWrite(d.triac_dim, LOW);
}

void balansun_pins_apply_boot() {
  pinMapFallbackActive = false;
  BalansunPinMap candidate = balansun_pin_map_from_stored();
  const bool s3 = balansun_target_is_s3();
  std::string err;
  if (!balansun_pin_map_validate(candidate, s3, err)) {
    candidate = balansun_board_default_pins();
    pinMapFallbackActive = true;
    Serial.printf("PIN_MAP: fallback to defaults (%s)\n", err.c_str());
  }
  g_pins = candidate;
  balansun_pins_assign_stored(g_pins);
  pinMode(g_pins.zero_cross, INPUT_PULLDOWN);
  pinMode(g_pins.triac_dim, OUTPUT);
  digitalWrite(g_pins.triac_dim, LOW);
  pinMapRebootPending = false;
}

void balansun_pins_reset_stored_to_defaults() {
  balansun_pins_assign_stored(balansun_board_default_pins());
  hwPinsPersistPresent = true;
}

void balansun_pins_update_reboot_pending() {
  BalansunPinMap stored = balansun_pin_map_from_stored();
  pinMapRebootPending = !balansun_pin_map_equal(stored, g_pins);
}

static bool read_core_pin_field(JsonObjectConst root, const char *key, int8_t &out) {
  if (root[key].isNull()) return false;
  const int v = root[key].as<int>();
  if (v < 0 || v > 48) return false;
  out = static_cast<int8_t>(v);
  return true;
}

static bool read_temp_pin_field(JsonObjectConst root, const char *key, int8_t &out) {
  if (root[key].isNull()) return false;
  const int v = root[key].as<int>();
  if (v < kBalansunPinTempDisabled || v > 48) return false;
  out = static_cast<int8_t>(v);
  return true;
}

static bool balansun_pins_reject_unapplicable_keys(JsonObjectConst root, String &err) {
  static const char *kPinKeys[] = {"pin_triac_dim", "pin_zero_cross", "pin_uart_rx", "pin_uart_tx",
                                   "pin_temp",      "pin_analog0",    "pin_analog1", "pin_analog2"};
  const SourceId eff = balansun_effective_meter_id();
  for (const char *key : kPinKeys) {
    if (root[key].isNull()) continue;
    if (balansun_source_logic_pin_field_allowed(key, eff)) continue;
    err = String(key) + " not applicable for measurement source " +
          balansun_source_logic_wire_for_id(eff);
    return false;
  }
  return true;
}

bool balansun_pins_apply_config_json_fields(JsonObjectConst root, String &err) {
  if (!balansun_pins_reject_unapplicable_keys(root, err)) return false;
  bool touched = false;
  int8_t nextTriac = pinTriacDim;
  int8_t nextZc = pinZeroCross;
  int8_t nextRx = pinUartRx;
  int8_t nextTx = pinUartTx;
  int8_t nextTemp = pinTempGpio;
  int8_t nextA0 = pinAnalog0;
  int8_t nextA1 = pinAnalog1;
  int8_t nextA2 = pinAnalog2;

  if (root["pin_triac_dim"].is<int>() && root["pin_triac_dim"].as<int>() == kBalansunPinTempDisabled) {
    err = "pin_triac_dim cannot be -1; use a GPIO number";
    return false;
  }
  if (root["pin_zero_cross"].is<int>() && root["pin_zero_cross"].as<int>() == kBalansunPinTempDisabled) {
    err = "pin_zero_cross cannot be -1; use a GPIO number";
    return false;
  }
  if (root["pin_uart_rx"].is<int>() && root["pin_uart_rx"].as<int>() == kBalansunPinTempDisabled) {
    err = "pin_uart_rx cannot be -1; use a GPIO number";
    return false;
  }
  if (root["pin_uart_tx"].is<int>() && root["pin_uart_tx"].as<int>() == kBalansunPinTempDisabled) {
    err = "pin_uart_tx cannot be -1; use a GPIO number";
    return false;
  }
  if (root["pin_analog0"].is<int>() && root["pin_analog0"].as<int>() == kBalansunPinTempDisabled) {
    err = "pin_analog0 cannot be -1; use a GPIO number";
    return false;
  }
  if (root["pin_analog1"].is<int>() && root["pin_analog1"].as<int>() == kBalansunPinTempDisabled) {
    err = "pin_analog1 cannot be -1; use a GPIO number";
    return false;
  }
  if (root["pin_analog2"].is<int>() && root["pin_analog2"].as<int>() == kBalansunPinTempDisabled) {
    err = "pin_analog2 cannot be -1; use a GPIO number";
    return false;
  }

  if (read_core_pin_field(root, "pin_triac_dim", nextTriac)) touched = true;
  if (read_core_pin_field(root, "pin_zero_cross", nextZc)) touched = true;
  if (read_core_pin_field(root, "pin_uart_rx", nextRx)) touched = true;
  if (read_core_pin_field(root, "pin_uart_tx", nextTx)) touched = true;
  if (read_temp_pin_field(root, "pin_temp", nextTemp)) touched = true;
  if (read_core_pin_field(root, "pin_analog0", nextA0)) touched = true;
  if (read_core_pin_field(root, "pin_analog1", nextA1)) touched = true;
  if (read_core_pin_field(root, "pin_analog2", nextA2)) touched = true;
  if (!touched) return true;

  BalansunPinMap trial;
  trial.triac_dim = nextTriac;
  trial.zero_cross = nextZc;
  trial.uart_rx = nextRx;
  trial.uart_tx = nextTx;
  trial.temp = nextTemp;
  trial.analog0 = nextA0;
  trial.analog1 = nextA1;
  trial.analog2 = nextA2;
  std::string errStd;
  if (!balansun_pin_map_validate(trial, balansun_target_is_s3(), errStd)) {
    err = String(errStd.c_str());
    return false;
  }
  pinTriacDim = nextTriac;
  pinZeroCross = nextZc;
  pinUartRx = nextRx;
  pinUartTx = nextTx;
  const int8_t prevTemp = pinTempGpio;
  pinTempGpio = nextTemp;
  if (pinTempGpio != prevTemp) {
    if (balansun_temp_logic_is_allowed_gpio(pinTempGpio)) {
      tempGpio = pinTempGpio;
    }
    balansun_temperature_invalidate_bus();
  }
  pinAnalog0 = nextA0;
  pinAnalog1 = nextA1;
  pinAnalog2 = nextA2;
  hwPinsPersistPresent = true;
  balansun_pins_update_reboot_pending();
  return true;
}

void balansun_pins_append_json(JsonObject o) {
  o["pin_triac_dim"] = pinTriacDim;
  o["pin_zero_cross"] = pinZeroCross;
  o["pin_uart_rx"] = pinUartRx;
  o["pin_uart_tx"] = pinUartTx;
  o["pin_temp"] = pinTempGpio;
  o["pin_analog0"] = pinAnalog0;
  o["pin_analog1"] = pinAnalog1;
  o["pin_analog2"] = pinAnalog2;
  o["pin_map_fallback_active"] = pinMapFallbackActive;
  o["pin_map_reboot_pending"] = pinMapRebootPending;
  o["board_pin_profile"] = balansun_board_pin_profile_id();
  const BalansunPinMap pinDefaults = balansun_board_default_pins();
  o["pin_default_triac_dim"] = pinDefaults.triac_dim;
  o["pin_default_zero_cross"] = pinDefaults.zero_cross;
  o["pin_default_uart_rx"] = pinDefaults.uart_rx;
  o["pin_default_uart_tx"] = pinDefaults.uart_tx;
  o["pin_default_temp"] = pinDefaults.temp;
  o["pin_default_analog0"] = pinDefaults.analog0;
  o["pin_default_analog1"] = pinDefaults.analog1;
  o["pin_default_analog2"] = pinDefaults.analog2;
}
