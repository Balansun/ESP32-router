#include "balansun_temperature.h"

#include "balansun_board.h"
#include "balansun_globals.h"
#include "balansun_hw_presence.h"
#include "balansun_pin_map.h"
#include "ds18b20_temperature_sensor.h"
#include "mqtt_ha_discovery.h"
#include "storage_eeprom_extension.h"

#include <balansun/ds18_poll.h>
#include <esp_task_wdt.h>

#include <string>

int tempGpio = pinTemp;
BalansunTempSlotConfig temperatureSlots[kBalansunTempMaxSensors];
BalansunTempSlotState g_temperature_slot_states[kBalansunTempMaxSensors];
int g_temperature_discovered_count = 0;
bool g_temperature_bus_active = false;

namespace {

Ds18b20TemperatureSensor g_ds18_sensor;

void mapReadingsToSlots(const TemperatureReading *readings, int count) {
  BalansunTempBusReading bus[kBalansunTempMaxSensors];
  for (int i = 0; i < kBalansunTempMaxSensors; i++) {
    bus[i] = {};
    if (i < count) {
      bus[i].c = readings[i].c;
      bus[i].valid = readings[i].valid;
      bus[i].address = readings[i].address;
    }
  }
  balansun_temp_logic_map_bus_to_slots(bus, count, temperatureSlots, g_temperature_slot_states);
}

}  // namespace

void balansun_temperature_init_defaults() {
  if (pinTempGpio != kBalansunPinTempDisabled) {
    tempGpio = pinTempGpio;
  } else {
    tempGpio = pinTemp;
  }
  temperatureSlots[0].enabled = true;
  temperatureSlots[0].label = temperatureSensorName.c_str();
  temperatureSlots[0].address = 0;
  temperatureSlots[1].enabled = false;
  temperatureSlots[1].label.clear();
  temperatureSlots[1].address = 0;
}

void balansun_temperature_sync_legacy_label() {
  if (!temperatureSlots[0].label.empty()) {
    temperatureSensorName = temperatureSlots[0].label.c_str();
  } else if (temperatureSensorName.length() > 0) {
    temperatureSlots[0].label = temperatureSensorName.c_str();
  }
}

int balansun_temperature_effective_gpio() {
  if (pinTempGpio == kBalansunPinTempDisabled) return -1;
  if (balansun_temp_logic_is_allowed_gpio(pinTempGpio)) return pinTempGpio;
  if (balansun_temp_logic_is_allowed_gpio(tempGpio)) return tempGpio;
  return pinTemp;
}

bool balansun_temperature_bus_should_run() {
  if (balansun_temperature_effective_gpio() < 0) return false;
  return balansun_temp_logic_any_slot_enabled(temperatureSlots);
}

void balansun_temperature_service(unsigned long now_ms) {
  if (!balansun_temperature_bus_should_run()) return;
  static unsigned long s_last_idle_poll_ms = 0;
  if (g_ds18_sensor.hasPendingConversion()) {
    Ds18PollState pending{};
    pending.last_request_ms = g_ds18_sensor.conversionRequestedMs();
    pending.pending = true;
    if (ds18_poll_conversion_ready(pending, static_cast<uint32_t>(now_ms))) {
      balansun_temperature_poll();
    }
    return;
  }
  if (s_last_idle_poll_ms == 0 || (now_ms - s_last_idle_poll_ms) >= 5000UL) {
    s_last_idle_poll_ms = now_ms;
    balansun_temperature_poll();
  }
}

void balansun_temperature_invalidate_bus() { g_ds18_sensor.invalidate(); }

void balansun_temperature_poll() {
  esp_task_wdt_reset();
  TemperatureReading readings[kBalansunTempMaxSensors];
  if (!balansun_temperature_bus_should_run()) {
    g_ds18_sensor.invalidate();
    g_temperature_discovered_count = 0;
    g_temperature_bus_active = false;
    mapReadingsToSlots(readings, 0);
    temperature = kBalansunTempInvalidC;
    balansun_hw_presence_on_temp_poll(false);
    return;
  }
  const int pin = balansun_temperature_effective_gpio();
  g_ds18_sensor.setup(pin);
  g_temperature_bus_active = true;
  int discovered = 0;
  const bool fresh = g_ds18_sensor.pollBusReadings(readings, kBalansunTempMaxSensors, discovered);
  if (!fresh && discovered == 0) {
    balansun_hw_presence_on_temp_poll(balansun_temp_logic_primary_slot(g_temperature_slot_states) >= 0);
    return;
  }
  g_temperature_discovered_count = discovered;
  mapReadingsToSlots(readings, discovered);
  temperature = balansun_temp_logic_primary_c(g_temperature_slot_states);
  balansun_hw_presence_on_temp_poll(balansun_temp_logic_primary_slot(g_temperature_slot_states) >= 0);
}

float getPrimaryTemperature() { return balansun_temp_logic_primary_c(g_temperature_slot_states); }

float getTemperature(int slot) {
  if (slot < 0 || slot >= kBalansunTempMaxSensors) return kBalansunTempInvalidC;
  const BalansunTempSlotState &st = g_temperature_slot_states[slot];
  if (!st.config.enabled || !st.reading.valid) return kBalansunTempInvalidC;
  return st.reading.c;
}

bool isTemperatureValid(int slot) {
  if (slot < 0 || slot >= kBalansunTempMaxSensors) return false;
  const BalansunTempSlotState &st = g_temperature_slot_states[slot];
  return st.config.enabled && st.reading.valid;
}

const char *activeTemperatureSensorKind() { return "ds18b20"; }

int balansun_temperature_active_valid_count() {
  int n = 0;
  for (int i = 0; i < kBalansunTempMaxSensors; i++) {
    if (g_temperature_slot_states[i].config.enabled && g_temperature_slot_states[i].reading.valid) n++;
  }
  return n;
}

void balansun_temperature_append_telemetry_json(JsonObject root) {
  JsonObject ts = root["temperature_sensors"].to<JsonObject>();
  ts["gpio"] = balansun_temperature_effective_gpio();
  ts["kind"] = activeTemperatureSensorKind();
  ts["max_count"] = kBalansunTempMaxSensors;
  ts["discovered_count"] = g_temperature_discovered_count;
  ts["bus_active"] = g_temperature_bus_active && balansun_temperature_bus_should_run();
  JsonArray sensors = ts["sensors"].to<JsonArray>();
  for (int s = 0; s < kBalansunTempMaxSensors; s++) {
    const BalansunTempSlotState &st = g_temperature_slot_states[s];
    JsonObject o = sensors.add<JsonObject>();
    o["slot"] = s;
    o["enabled"] = st.config.enabled;
    if (!st.config.label.empty()) {
      o["label"] = st.config.label;
    } else {
      o["label"] = nullptr;
    }
    const std::string addr = balansun_temp_logic_format_address(st.reading.address);
    if (!addr.empty()) {
      o["address"] = addr;
    } else {
      o["address"] = nullptr;
    }
    if (st.config.enabled && st.reading.valid) {
      o["temperature_c"] = st.reading.c;
      o["valid"] = true;
    } else {
      o["temperature_c"] = nullptr;
      o["valid"] = false;
    }
    o["primary"] = st.primary;
  }
}

void balansun_temperature_append_config_json(JsonObject root) {
  root["temp_gpio"] = balansun_temperature_effective_gpio();
  JsonArray slots = root["temperature_slots"].to<JsonArray>();
  for (int i = 0; i < kBalansunTempMaxSensors; i++) {
    JsonObject o = slots.add<JsonObject>();
    o["enabled"] = temperatureSlots[i].enabled;
    o["label"] = temperatureSlots[i].label.empty() ? "" : temperatureSlots[i].label.c_str();
    o["address"] = balansun_temp_logic_format_address(temperatureSlots[i].address);
  }
}

static bool parse_address_hex(const char *hex, uint64_t &out) {
  out = 0;
  if (!hex || !hex[0]) return true;
  if (strlen(hex) != 16) return false;
  for (int i = 0; i < 16; i++) {
    char c = hex[i];
    int v = 0;
    if (c >= '0' && c <= '9')
      v = c - '0';
    else if (c >= 'A' && c <= 'F')
      v = c - 'A' + 10;
    else if (c >= 'a' && c <= 'f')
      v = c - 'a' + 10;
    else
      return false;
    out = (out << 4) | static_cast<uint64_t>(v);
  }
  return true;
}

bool balansun_temperature_apply_config_json(JsonObject root, bool has_slots, bool has_gpio, bool has_legacy_label,
                                         String &err) {
  if (has_gpio) {
    const int g = root["temp_gpio"].as<int>();
    std::string gpioErr;
    if (!balansun_temp_logic_validate_gpio(g, gpioErr)) {
      err = gpioErr.c_str();
      return false;
    }
    tempGpio = g;
    if (balansun_temp_logic_is_allowed_gpio(g)) {
      pinTempGpio = static_cast<int8_t>(g);
    }
    balansun_temperature_invalidate_bus();
  }
  if (has_slots) {
    JsonArray arr = root["temperature_slots"].as<JsonArray>();
    if (arr.size() > static_cast<size_t>(kBalansunTempMaxSensors)) {
      err = "temperature_slots max 2";
      return false;
    }
    for (size_t i = 0; i < arr.size() && i < static_cast<size_t>(kBalansunTempMaxSensors); i++) {
      JsonObject o = arr[i].as<JsonObject>();
      if (!o["enabled"].isNull()) temperatureSlots[i].enabled = o["enabled"].as<bool>();
      if (!o["label"].isNull()) temperatureSlots[i].label = o["label"].as<const char *>();
      if (!o["address"].isNull()) {
        const char *addr = o["address"].as<const char *>();
        uint64_t packed = 0;
        if (!parse_address_hex(addr, packed)) {
          err = "temperature_slots.address must be 16 hex chars or empty";
          return false;
        }
        temperatureSlots[i].address = packed;
      }
    }
  }
  if (has_legacy_label) {
    temperatureSlots[0].label = root["temperature_label"].as<const char *>();
  }
  balansun_temperature_sync_legacy_label();
  mqtt_ha_discovered = false;
  return true;
}

void balansun_temperature_load_extension_fields(const EepromExtensionFields &fields) {
  if (!fields.temperaturePersistPresent) return;
  if (fields.tempGpio >= 0 && balansun_temp_logic_is_allowed_gpio(fields.tempGpio)) {
    tempGpio = fields.tempGpio;
  }
  for (int i = 0; i < kBalansunTempMaxSensors && i < EepromExtensionFields::kTemperatureSlotStoredMax; i++) {
    temperatureSlots[i].enabled = fields.temperatureSlots[i].enabled;
    temperatureSlots[i].label = fields.temperatureSlots[i].label;
    temperatureSlots[i].address = fields.temperatureSlots[i].address;
  }
  balansun_temperature_sync_legacy_label();
}

void balansun_temperature_store_extension_fields(EepromExtensionFields &fields) {
  fields.temperaturePersistPresent = true;
  fields.tempGpio = static_cast<int8_t>(balansun_temperature_effective_gpio());
  for (int i = 0; i < kBalansunTempMaxSensors && i < EepromExtensionFields::kTemperatureSlotStoredMax; i++) {
    fields.temperatureSlots[i].enabled = temperatureSlots[i].enabled;
    fields.temperatureSlots[i].label = temperatureSlots[i].label;
    fields.temperatureSlots[i].address = temperatureSlots[i].address;
  }
}
