#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_VictronGx
#include "victron_gx_mqtt.h"

#include "balansun_globals.h"
#include "balansun_pub.h"
#include "balansun_source.h"
#include "api_util.h"
#include "victron_gx_logic.h"

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <vector>

namespace {

WiFiClient g_victronWifiClient;
PubSubClient g_victronMqtt(g_victronWifiClient);

VictronRawInputs g_raw;
float g_grid_phase_w[3] = {0.0f, 0.0f, 0.0f};
bool g_grid_phase_ok[3] = {false, false, false};
unsigned long g_lastBatteryRxMs = 0;
unsigned long g_lastGridRxMs = 0;
unsigned long g_lastConnectAttemptMs = 0;
bool g_loggedConnected = false;

constexpr unsigned long kConnectBackoffMs = 5000UL;
constexpr uint32_t kSocketTimeoutMs = 2500;

bool victron_active(void) { return balansun_active_source_get() == SourceId::VictronGx; }

String portal_id(void) {
  String id = VictronPortalId.c_str();
  id.trim();
  return id;
}

bool broker_configured(void) {
  const String host = ip32ToDotted(victron_broker_ip);
  return host.length() > 0 && host != "0.0.0.0" && portal_id().length() > 0;
}

VictronSurplusMode surplus_mode(void) {
  VictronSurplusMode mode = VictronSurplusMode::BatteryCharge;
  balansun_victron_surplus_mode_from_wire(VictronSurplusModeWire.c_str(), mode);
  return mode;
}

bool grid_topics_needed(void) {
  const VictronSurplusMode m = surplus_mode();
  return m == VictronSurplusMode::GridExport || m == VictronSurplusMode::CombinedMax;
}

void recompute_grid_sum(void) {
  g_raw.grid_w = 0.0f;
  g_raw.have_grid = false;
  const uint8_t phases = victron_grid_phases < 1 ? 1 : (victron_grid_phases > 3 ? 3 : victron_grid_phases);
  for (uint8_t p = 0; p < phases; p++) {
    if (!g_grid_phase_ok[p]) return;
    g_raw.grid_w += g_grid_phase_w[p];
    g_raw.have_grid = true;
  }
}

void apply_surplus_to_globals(void) {
  const VictronHousePower hp = balansun_victron_map_surplus(g_raw, surplus_mode());
  house_active_import_w = hp.active_import_w;
  house_active_export_w = hp.active_export_w;
  house_apparent_import_va = hp.active_import_w;
  house_apparent_export_va = hp.active_export_w;
  meter_reading_valid = true;
  LastVictronMqttMillis = millis();
  last_metering_task_ms = millis();
  BalansunPublishFromGlobals();
}

bool topic_is_battery(const String &topic) { return topic.endsWith("/Dc/0/Power"); }

bool topic_is_grid(const String &topic) { return topic.indexOf("/Ac/Grid/L") >= 0 && topic.endsWith("/Power"); }

void on_victron_message(char *topic, byte *payload, unsigned int length) {
  if (!victron_active()) return;
  String msg;
  msg.reserve(length + 1);
  for (unsigned int i = 0; i < length; i++) msg += static_cast<char>(payload[i]);
  float value = 0.0f;
  if (!balansun_victron_parse_value_json(std::string(msg.c_str()), value)) return;

  const String topicStr(topic);
  const unsigned long now = millis();
  if (topic_is_battery(topicStr)) {
    g_raw.have_battery = true;
    g_raw.battery_w = value;
    g_lastBatteryRxMs = now;
  } else if (topic_is_grid(topicStr)) {
    if (!grid_topics_needed()) return;
    int phaseIdx = -1;
    const int lpos = topicStr.indexOf("/Ac/Grid/L");
    if (lpos >= 0 && lpos + 11 < topicStr.length()) {
      const char phaseChar = topicStr.charAt(lpos + 11);
      if (phaseChar >= '1' && phaseChar <= '3') phaseIdx = phaseChar - '1';
    }
    if (phaseIdx < 0 || phaseIdx > 2) return;
    g_grid_phase_w[phaseIdx] = value;
    g_grid_phase_ok[phaseIdx] = true;
    recompute_grid_sum();
    g_lastGridRxMs = now;
  } else {
    return;
  }
  apply_surplus_to_globals();
}

void subscribe_topics(void) {
  const String pid = portal_id();
  if (pid.length() == 0) return;
  String batteryTopic = "N/" + pid + "/battery/" + String(victron_battery_device_id) + "/Dc/0/Power";
  g_victronMqtt.subscribe(batteryTopic.c_str());
  if (grid_topics_needed()) {
    const uint8_t phases = victron_grid_phases < 1 ? 1 : (victron_grid_phases > 3 ? 3 : victron_grid_phases);
    for (uint8_t p = 0; p < 3; p++) {
      g_grid_phase_w[p] = 0.0f;
      g_grid_phase_ok[p] = false;
    }
    g_raw.have_grid = false;
    g_raw.grid_w = 0.0f;
    for (uint8_t p = 1; p <= phases; p++) {
      String gridTopic = "N/" + pid + "/system/0/Ac/Grid/L" + String(p) + "/Power";
      g_victronMqtt.subscribe(gridTopic.c_str());
    }
  }
}

bool ensure_connected(void) {
  if (!victron_active() || !broker_configured()) return false;
  if (g_victronMqtt.connected()) return true;
  const unsigned long now = millis();
  if (g_lastConnectAttemptMs != 0 && (now - g_lastConnectAttemptMs) < kConnectBackoffMs) return false;
  g_lastConnectAttemptMs = now;
  const String host = ip32ToDotted(victron_broker_ip);
  g_victronWifiClient.setTimeout(kSocketTimeoutMs);
  g_victronMqtt.setServer(host.c_str(), 1883);
  g_victronMqtt.setCallback(on_victron_message);
  g_victronMqtt.setBufferSize(512);
  String clientId = String(MQTTdeviceName.c_str()) + "-victron";
  if (!g_victronMqtt.connect(clientId.c_str())) {
    if (g_loggedConnected) {
      g_loggedConnected = false;
      Serial.println("Victron MQTT connection failed");
    }
    return false;
  }
  if (!g_loggedConnected) {
    g_loggedConnected = true;
    Serial.print("Victron MQTT connected to ");
    Serial.println(host);
  }
  subscribe_topics();
  return true;
}

}  // namespace

void victron_gx_mqtt_begin(void) {
  g_raw = VictronRawInputs{};
  for (int i = 0; i < 3; i++) {
    g_grid_phase_w[i] = 0.0f;
    g_grid_phase_ok[i] = false;
  }
  g_lastBatteryRxMs = 0;
  g_lastGridRxMs = 0;
  g_lastConnectAttemptMs = 0;
  g_loggedConnected = false;
  if (g_victronMqtt.connected()) g_victronMqtt.disconnect();
}

void victron_gx_mqtt_reconnect(void) {
  if (g_victronMqtt.connected()) g_victronMqtt.disconnect();
  g_lastConnectAttemptMs = 0;
  g_loggedConnected = false;
  g_raw = VictronRawInputs{};
}

void victron_gx_mqtt_service_tick(void) {
  if (!victron_active()) {
    if (g_victronMqtt.connected()) g_victronMqtt.disconnect();
    return;
  }
  if (!ensure_connected()) return;
  for (int i = 0; i < 4; i++) {
    g_victronMqtt.loop();
    yield();
  }
}

void victron_gx_mqtt_append_diagnostics(JsonObject doc, int linky_tail_max) {
  (void)linky_tail_max;
  JsonObject vx = doc["victron_gx"].to<JsonObject>();
  vx["broker_ip"] = ip32ToDotted(victron_broker_ip);
  vx["portal_id"] = VictronPortalId.c_str();
  vx["battery_device_id"] = victron_battery_device_id;
  vx["surplus_mode"] = VictronSurplusModeWire.c_str();
  vx["grid_phases"] = victron_grid_phases;
  vx["mqtt_connected"] = g_victronMqtt.connected();
  if (g_raw.have_battery) vx["battery_power_w"] = g_raw.battery_w;
  if (g_raw.have_grid) vx["grid_power_w"] = g_raw.grid_w;
  if (g_lastBatteryRxMs > 0) {
    vx["battery_last_rx_ms_ago"] = static_cast<int>((millis() - g_lastBatteryRxMs) & 0x7FFFFFFF);
  }
  if (g_lastGridRxMs > 0) {
    vx["grid_last_rx_ms_ago"] = static_cast<int>((millis() - g_lastGridRxMs) & 0x7FFFFFFF);
  }
}

#else /* !BALANSUN_ENABLE_SOURCE_VictronGx */

#include "victron_gx_mqtt.h"

void victron_gx_mqtt_begin(void) {}
void victron_gx_mqtt_reconnect(void) {}
void victron_gx_mqtt_service_tick(void) {}
void victron_gx_mqtt_append_diagnostics(JsonObject, int) {}

#endif
