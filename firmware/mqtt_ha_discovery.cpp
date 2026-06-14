/*
 * mqtt_ha_discovery.cpp — Home Assistant MQTT auto-discovery entity configs.
 */
#include "balansun_globals.h"
#include "balansun_diag.h"
#include <cstdio>
#include "balansun_board.h"
#include "balansun_device_id.h"
#include "mqtt_ha_topics.h"
#include "mqtt_ha_discovery.h"
#include "balansun_source.h"
#include "tempo_rte_logic.h"
#include "mqtt_ha_logic.h"
#include "balansun_source_health_logic.h"
#include "storage_eeprom.h"
#include <ArduinoJson.h>
#include "balansun_json.h"
#include "balansun_temperature.h"

bool mqtt_ha_discovered = false;

// Component types required by Home Assistant auto-discovery.
const char *SSR = "sensor";
const char *SLCT = "select";
const char *NB = "number";
const char *BINS = "binary_sensor";
const char *SWTC = "switch";
const char *DAUT = "device_automation";
const char *TXT = "text";
static void mqttDocAddAvty(JsonDocument &doc) {
  doc["avty_t"] = mqttAvailabilityTopic();
  doc["pl_avail"] = "online";
  doc["pl_not_avail"] = "offline";
}

/** HA device block: identifiers = device_uid; WiFi MAC in connections. */
static void mqttDocFillHaDevice(JsonObject device, bool fullMetadata) {
  const String &uid = balansun_device_uid();
  device["ids"][0] = uid;
  device["serial_number"] = uid;
  JsonArray conn = device["connections"].to<JsonArray>();
  JsonArray macConn = conn.add<JsonArray>();
  macConn.add("mac");
  macConn.add(WiFi.macAddress());
  if (fullMetadata) {
    device["cu"] = "http://" + WiFi.localIP().toString();
    device["hw"] = String(ESP.getChipModel()) + " rev." + String(ESP.getChipRevision());
    device["mf"] = "Balansun";
    device["mdl"] = "ESP32 - " + uid;
    device["name"] = routerName;
    device["sw"] = Version;
  }
}

/** HA listens on homeassistant/<component>/…/config by default (state/cmd use MQTTPrefix). */
static String mqttDiscoveryTopic(const char *component, const String &objectId) {
  return String(mqtt_ha_logic_discovery_prefix()) + "/" + component + "/" + objectId + "/config";
}

static bool mqttPublishDiscovery(const char *topic, const void *payload, size_t len) {
  return clientMQTT.publish(
      topic, static_cast<const uint8_t *>(payload), static_cast<unsigned int>(len), true);
}

static String mqttIsoNow() {
  time_t now = time(NULL);
  if (now < 1000) return "1970-01-01T00:00:00Z";
  char buf[25];
  struct tm *utc = gmtime(&now);
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", utc);
  return String(buf);
}

/** Human-readable HA entity name from probe labels (stable object id unchanged). */
static String mqttHaMetricFriendlyName(const String &key) {
  const String houseLabel =
      probeHouseName.length() > 0 ? String(probeHouseName.c_str()) : String("House");
  const String secondLabel =
      probeSecondName.length() > 0 ? String(probeSecondName.c_str()) : String("Second channel");
  if (key == "house_net_power_w") return houseLabel + " net power";
  if (key == "house_active_import_w") return houseLabel + " active import";
  if (key == "house_active_export_w") return houseLabel + " active export";
  if (key == "house_voltage_v") return houseLabel + " voltage";
  if (key == "house_current_a") return houseLabel + " current";
  if (key == "house_power_factor") return houseLabel + " power factor";
  if (key == "house_energy_import_wh") return houseLabel + " energy import";
  if (key == "house_energy_export_wh") return houseLabel + " energy export";
  if (key == "house_day_energy_import_wh") return houseLabel + " day energy import";
  if (key == "house_day_energy_export_wh") return houseLabel + " day energy export";
  if (key == "second_active_import_w") return secondLabel + " active import";
  if (key == "second_active_export_w") return secondLabel + " active export";
  if (key == "second_voltage_v") return secondLabel + " voltage";
  if (key == "second_current_a") return secondLabel + " current";
  if (key == "second_power_factor") return secondLabel + " power factor";
  if (key == "second_energy_import_wh") return secondLabel + " energy import";
  if (key == "second_energy_export_wh") return secondLabel + " energy export";
  if (key == "second_day_energy_import_wh") return secondLabel + " day energy import";
  if (key == "second_day_energy_export_wh") return secondLabel + " day energy export";
  if (key == "mains_frequency_hz") return String("Mains frequency");
  if (key == "temperature_c") return String("Temperature");
  return String(MQTTdeviceName) + " " + key;
}
void sendMQTTDiscoveryMsg_global() {
  // Enlarge MQTT WiFi buffer (see PubSubClient.h)
  clientMQTT.setBufferSize(700);  // voir -->#define MQTT_MAX_PACKET_SIZE 256 is the default value in PubSubClient.h
  if (balansun_cap_mqtt_triac_channel_block()) {
    DeviceToDiscover("second_active_import_w", "W", "power", "0");
    DeviceToDiscover("second_active_export_w", "W", "power", "0");
    DeviceToDiscover("second_voltage_v", "V", "voltage", "2");
    DeviceToDiscover("second_current_a", "A", "current", "2");
    DeviceToDiscover("second_power_factor", "", "power_factor", "2");
    DeviceToDiscover("second_energy_import_wh", "Wh", "energy", "0");
    DeviceToDiscover("second_energy_export_wh", "Wh", "energy", "0");
    DeviceToDiscover("second_day_energy_import_wh", "Wh", "energy", "0");
    DeviceToDiscover("second_day_energy_export_wh", "Wh", "energy", "0");
    DeviceToDiscover("mains_frequency_hz", "Hz", "frequency", "2");
  }
  for (int s = 0; s < kBalansunTempMaxSensors; s++) {
    const BalansunTempSlotState &st = g_temperature_slot_states[s];
    if (!st.config.enabled || !st.reading.valid) continue;
    if (st.primary) {
      DeviceToDiscover("temperature_c", "°C", "temperature", "1");
    } else {
      char key[24];
      snprintf(key, sizeof(key), "temperature_slot_%d_c", s);
      DeviceToDiscover(key, "°C", "temperature", "1");
    }
  }
  
  if (balansun_cap_mqtt_linky_tariff()) {
    DeviceTextToDiscover("linky_ltarf", "Option Tarifaire");
    DeviceToDiscover("tariff_code", "", "", "0");
  }
  if (tempoRteEnabled) {
    DeviceTextToDiscover("rte_today", "RTE today");
    DeviceTextToDiscover("rte_tomorrow", "RTE tomorrow");
  }

  DeviceToDiscover("house_net_power_w", "W", "power", "0");
  DeviceToDiscover("house_active_import_w", "W", "power", "0");
  DeviceToDiscover("house_active_export_w", "W", "power", "0");
  DeviceToDiscover("house_voltage_v", "V", "voltage", "2");
  DeviceToDiscover("house_current_a", "A", "current", "2");
  DeviceToDiscover("house_power_factor", "", "power_factor", "2");
  DeviceToDiscover("house_energy_import_wh", "Wh", "energy", "0");
  DeviceToDiscover("house_energy_export_wh", "Wh", "energy", "0");
  DeviceToDiscover("house_day_energy_import_wh", "Wh", "energy", "0");
  DeviceToDiscover("house_day_energy_export_wh", "Wh", "energy", "0");

  DeviceToDiscover("triac_open_percent", "%", "power_factor", "0");  // HA accepts power_factor for 0–100 % triac opening
  DeviceBinToDiscover("adc_clipping", "Analog ADC clipping");
  DeviceBinToDiscover("regulation_hunting", "Regulation hunting");
  DeviceBinToDiscover("source_stale", "Source stale");
  DeviceBinToDiscover("regulation_active", "Regulation active");
  DeviceBinToDiscover("mqtt_connected", "MQTT connected");
  DeviceBinToDiscover("site_cap_active", "Site power cap active");
  DeviceBinToDiscover("heater_load_backoff_active", "Routed load backoff active");
  DeviceSensorNumberToDiscover("source_health", "Source health", 0, 100);
  DeviceNumberToDiscover("triac_target", "Target triac opening", 0, 100);
  DeviceConfigNumberToDiscover("max_routed_w", "Max routed power W", "max_routed_w/set", 0, 20000);
  DeviceSelectSourceToDiscover();
  for (int i = 1; i < NbActions; i++) {
    DeviceSwitchToDiscover(i, load_channels[i].title);
  }
  DeviceVacationSwitchToDiscover();

  DeviceAutomationTriggerToDiscover("source_lost", "Source lost");
  DeviceAutomationTriggerToDiscover("regulation_hunting", "Regulation hunting");
  DeviceAutomationTriggerToDiscover("vacation_ended", "Vacation ended");
  DeviceAutomationTriggerToDiscover("action_cap_hit", "Action daily cap hit");
  DeviceAutomationTriggerToDiscover("surplus_started", "Surplus routing started");
  DeviceAutomationTriggerToDiscover("surplus_ended", "Surplus routing ended");
  DeviceAutomationTriggerToDiscover("triac_cap_hit", "Site power cap hit");
  if (balansun_cap_mqtt_linky_tariff()) {
    DeviceAutomationTriggerToDiscover("linky_tariff_changed", "Linky tariff changed");
  }

  Serial.println("Home Assistant auto-discovery published.");
  Debug.println("Home Assistant auto-discovery published.");

  //clientMQTT.setBufferSize(512);  // go to initial value wifi/mqtt buffer
  mqtt_ha_discovered = true;


}  // END OF sendMQTTDiscoveryMsg_global

void DeviceToDiscover(String Name, String Unit, String Class, String Round) {

  String StateTopic = String(MQTTPrefix) + "/" + MQTTdeviceName + "_state";
  BalansunJsonDoc _balansunJsonPool1 = balansun_json_doc_alloc(512);
  JsonDocument &doc = _balansunJsonPool1;  // this is the Payload json format
  JsonObject device;             // for device object  "device": {}
  JsonArray option;              // options (array) of this device
  char buffer[512];
  size_t n;
  bool published;

  String DiscoveryTopic;              // HA discovery config topic for this entity

  //DiscoveryTopic = ConfigTopic(Name, SSR, "config");
  DiscoveryTopic = mqttDiscoveryTopic(SSR, String(MQTTdeviceName) + "_" + String(Name));
  doc["name"] = mqttHaMetricFriendlyName(Name);
  doc["uniq_id"] = String(MQTTdeviceName) + "_" + String(Name);  // Unique id (stable across reboots)
  doc["stat_t"] = StateTopic;
  doc["unit_of_meas"] = Unit;
  if (Unit == "W") {
    doc["state_class"] = "measurement";
  }
  if (Unit=="Wh"){
      doc["state_class"] = "total_increasing";
      doc["last_reset"] = mqttIsoNow();
  }
  doc["device_class"] = Class;
  doc["val_tpl"] = "{{ value_json." + Name + "|default(0)| round(" + Round + ") }}";
  device = doc["device"].to<JsonObject>();
  mqttDocFillHaDevice(device, true);
  mqttDocAddAvty(doc);

  n = serializeJson(doc, buffer);
  published = mqttPublishDiscovery(DiscoveryTopic.c_str(), buffer, n);
  doc.clear();
  buffer[0] = '\0';
}
void DeviceBinToDiscover(String Name, String title) {

  String StateTopic = String(MQTTPrefix) + "/" + MQTTdeviceName + "_state";
  BalansunJsonDoc _balansunJsonPool2 = balansun_json_doc_alloc(512);
  JsonDocument &doc = _balansunJsonPool2;  // this is the Payload json format
  JsonObject device;             // for device object  "device": {}
  JsonArray option;              // options (array) of this device
  char buffer[512];
  size_t n;
  bool published;

  String DiscoveryTopic;              // HA discovery config topic for this entity

 
  DiscoveryTopic = mqttDiscoveryTopic(BINS, String(MQTTdeviceName) + "_" + String(Name));
  doc["name"] = String(MQTTdeviceName) + " " + String(title);    // Friendly entity name
  doc["uniq_id"] = String(MQTTdeviceName) + "_" + String(Name);  // Unique id (stable across reboots)
  doc["stat_t"] = StateTopic;
  doc["init"] = "OFF";  // default value
  doc["ic"] = "mdi:electric-switch";
  doc["val_tpl"] = "{{ value_json." + Name + " }}";
  device = doc["device"].to<JsonObject>();
  mqttDocFillHaDevice(device, false);
  mqttDocAddAvty(doc);
  n = serializeJson(doc, buffer);
  published = mqttPublishDiscovery(DiscoveryTopic.c_str(), buffer, n);
  doc.clear();
  buffer[0] = '\0';
}

void DeviceAutomationTriggerToDiscover(const char *subtype, const char *title) {
  BalansunJsonDoc _balansunJsonPool3 = balansun_json_doc_alloc(512);
  JsonDocument &doc = _balansunJsonPool3;
  char buffer[512];
  String objectId = String(MQTTdeviceName) + "_evt_" + String(subtype);
  String discoveryTopic = mqttDiscoveryTopic(DAUT, objectId);
  doc["automation_type"] = "trigger";
  doc["type"] = "balansun";
  doc["subtype"] = subtype;
  doc["name"] = String(MQTTdeviceName) + " " + String(title);
  doc["topic"] = mqttEventTopic();
  doc["payload"] = subtype;
  doc["value_template"] = "{{ value_json.type }}";
  JsonObject device = doc["device"].to<JsonObject>();
  mqttDocFillHaDevice(device, false);
  mqttDocAddAvty(doc);
  size_t n = serializeJson(doc, buffer);
  mqttPublishDiscovery(discoveryTopic.c_str(), buffer, n);
}

void DeviceVacationSwitchToDiscover() {
  String StateTopic = mqttStateTopic();
  String CommandTopic = String(MQTTPrefix) + "/" + MQTTdeviceName + "/vacation/set";
  BalansunJsonDoc _balansunJsonPool4 = balansun_json_doc_alloc(700);
  JsonDocument &doc = _balansunJsonPool4;
  char buffer[700];
  String DiscoveryTopic = mqttDiscoveryTopic(SWTC, String(MQTTdeviceName) + "_vacation");
  doc["name"] = String(MQTTdeviceName) + " Vacation";
  doc["uniq_id"] = String(MQTTdeviceName) + "_vacation";
  doc["stat_t"] = StateTopic;
  doc["cmd_t"] = CommandTopic;
  doc["val_tpl"] = "{{ value_json.vacation }}";
  doc["pl_on"] = "ON";
  doc["pl_off"] = "OFF";
  JsonObject device = doc["device"].to<JsonObject>();
  mqttDocFillHaDevice(device, false);
  mqttDocAddAvty(doc);
  size_t n = serializeJson(doc, buffer);
  mqttPublishDiscovery(DiscoveryTopic.c_str(), buffer, n);
}

void DeviceSwitchToDiscover(int index, String title) {
  String StateTopic = mqttStateTopic();
  String CommandTopic = String(MQTTPrefix) + "/" + MQTTdeviceName + "/action_" + String(index) + "/set";
  BalansunJsonDoc _balansunJsonPool5 = balansun_json_doc_alloc(700);
  JsonDocument &doc = _balansunJsonPool5;
  char buffer[700];
  String name = "Action_" + String(index);
  String DiscoveryTopic = mqttDiscoveryTopic(SWTC, String(MQTTdeviceName) + "_" + name);
  doc["name"] = String(MQTTdeviceName) + " " + title;
  doc["uniq_id"] = String(MQTTdeviceName) + "_" + name;
  doc["stat_t"] = StateTopic;
  doc["cmd_t"] = CommandTopic;
  doc["avty_t"] = mqttAvailabilityTopic();
  doc["pl_avail"] = "online";
  doc["pl_not_avail"] = "offline";
  doc["val_tpl"] = "{{ value_json." + name + " }}";
  doc["pl_on"] = "ON";
  doc["pl_off"] = "OFF";
  doc["stat_on"] = "ON";
  doc["stat_off"] = "OFF";
  JsonObject device = doc["device"].to<JsonObject>();
  mqttDocFillHaDevice(device, false);
  device["name"] = routerName;
  size_t n = serializeJson(doc, buffer);
  mqttPublishDiscovery(DiscoveryTopic.c_str(), buffer, n);
}

void DeviceSensorNumberToDiscover(const String &Name, const String &title, int minValue, int maxValue) {
  String StateTopic = mqttStateTopic();
  BalansunJsonDoc _balansunJsonPool6 = balansun_json_doc_alloc(512);
  JsonDocument &doc = _balansunJsonPool6;
  char buffer[512];
  String DiscoveryTopic = mqttDiscoveryTopic(SSR, String(MQTTdeviceName) + "_" + Name);
  doc["name"] = String(MQTTdeviceName) + " " + title;
  doc["uniq_id"] = String(MQTTdeviceName) + "_" + Name;
  doc["stat_t"] = StateTopic;
  doc["val_tpl"] = "{{ value_json." + Name + "|default(0)|int }}";
  doc["min"] = minValue;
  doc["max"] = maxValue;
  JsonObject device = doc["device"].to<JsonObject>();
  mqttDocFillHaDevice(device, false);
  mqttDocAddAvty(doc);
  size_t n = serializeJson(doc, buffer);
  mqttPublishDiscovery(DiscoveryTopic.c_str(), buffer, n);
}

void DeviceConfigNumberToDiscover(const String &Name, const String &title, const String &cmdSuffix, int minValue,
                                         int maxValue) {
  String StateTopic = mqttStateTopic();
  String CommandTopic = String(MQTTPrefix) + "/" + MQTTdeviceName + "/" + cmdSuffix;
  BalansunJsonDoc _balansunJsonPool7 = balansun_json_doc_alloc(700);
  JsonDocument &doc = _balansunJsonPool7;
  char buffer[700];
  String DiscoveryTopic = mqttDiscoveryTopic(NB, String(MQTTdeviceName) + "_" + Name);
  doc["name"] = String(MQTTdeviceName) + " " + title;
  doc["uniq_id"] = String(MQTTdeviceName) + "_" + Name;
  doc["stat_t"] = StateTopic;
  doc["cmd_t"] = CommandTopic;
  doc["min"] = minValue;
  doc["max"] = maxValue;
  doc["mode"] = "box";
  doc["val_tpl"] = "{{ value_json." + Name + "|default(0)|int }}";
  JsonObject device = doc["device"].to<JsonObject>();
  mqttDocFillHaDevice(device, false);
  mqttDocAddAvty(doc);
  size_t n = serializeJson(doc, buffer);
  mqttPublishDiscovery(DiscoveryTopic.c_str(), buffer, n);
}

void DeviceNumberToDiscover(String Name, String title, int minValue, int maxValue) {
  String StateTopic = mqttStateTopic();
  String CommandTopic = String(MQTTPrefix) + "/" + MQTTdeviceName + "/triac/set";
  BalansunJsonDoc _balansunJsonPool8 = balansun_json_doc_alloc(700);
  JsonDocument &doc = _balansunJsonPool8;
  char buffer[700];
  String DiscoveryTopic = mqttDiscoveryTopic(NB, String(MQTTdeviceName) + "_" + Name);
  doc["name"] = String(MQTTdeviceName) + " " + title;
  doc["uniq_id"] = String(MQTTdeviceName) + "_" + Name;
  doc["stat_t"] = StateTopic;
  doc["cmd_t"] = CommandTopic;
  doc["avty_t"] = mqttAvailabilityTopic();
  doc["unit_of_meas"] = "%";
  doc["min"] = minValue;
  doc["max"] = maxValue;
  doc["mode"] = "slider";
  doc["val_tpl"] = "{{ value_json.triac_open_percent|default(0)| round(0) }}";
  JsonObject device = doc["device"].to<JsonObject>();
  mqttDocFillHaDevice(device, false);
  device["name"] = routerName;
  size_t n = serializeJson(doc, buffer);
  mqttPublishDiscovery(DiscoveryTopic.c_str(), buffer, n);
}

void DeviceSelectSourceToDiscover() {
  String StateTopic = mqttStateTopic();
  String CommandTopic = String(MQTTPrefix) + "/" + MQTTdeviceName + "/source/set";
  BalansunJsonDoc _balansunJsonPool9 = balansun_json_doc_alloc(700);
  JsonDocument &doc = _balansunJsonPool9;
  char buffer[700];
  String DiscoveryTopic = mqttDiscoveryTopic(SLCT, String(MQTTdeviceName) + "_Source");
  doc["name"] = String(MQTTdeviceName) + " Source";
  doc["uniq_id"] = String(MQTTdeviceName) + "_Source";
  doc["stat_t"] = StateTopic;
  doc["cmd_t"] = CommandTopic;
  doc["avty_t"] = mqttAvailabilityTopic();
  doc["val_tpl"] = "{{ value_json.source }}";
  JsonArray options = doc["options"].to<JsonArray>();
  for (size_t i = 0; i < balansun_source_registry_count(); i++) {
    options.add(balansun_source_wire_at(i));
  }
  JsonObject device = doc["device"].to<JsonObject>();
  mqttDocFillHaDevice(device, false);
  device["name"] = routerName;
  size_t n = serializeJson(doc, buffer);
  mqttPublishDiscovery(DiscoveryTopic.c_str(), buffer, n);
}

void DeviceTextToDiscover(String Name, String title) {
  String StateTopic = String(MQTTPrefix) + "/" + MQTTdeviceName + "_state";
  BalansunJsonDoc _balansunJsonPool10 = balansun_json_doc_alloc(512);
  JsonDocument &doc = _balansunJsonPool10;  // this is the Payload json format
  JsonObject device;             // for device object  "device": {}
  JsonArray option;              // options (array) of this device
  char buffer[512];
  size_t n;
  bool published;
  String DiscoveryTopic;              // HA discovery config topic for this entity
  DiscoveryTopic = mqttDiscoveryTopic(TXT, String(MQTTdeviceName) + "_" + String(Name));
  doc["name"] = String(MQTTdeviceName) + " " + String(title);    // Friendly entity name
  doc["uniq_id"] = String(MQTTdeviceName) + "_" + String(Name);  // Unique id (stable across reboots)
  doc["stat_t"] = StateTopic;
  doc["val_tpl"] = "{{ value_json." + Name + " }}";
  device = doc["device"].to<JsonObject>();
  mqttDocFillHaDevice(device, false);
  mqttDocAddAvty(doc);
  n = serializeJson(doc, buffer);
  published = mqttPublishDiscovery(DiscoveryTopic.c_str(), buffer, n);
  doc.clear();
  buffer[0] = '\0';
}
