#include "action_node_app.h"

#include <balansun/ds18_poll.h>

#include <DallasTemperature.h>
#include <ESP8266WebServer.h>
#include <OneWire.h>
#include <WiFiClient.h>

extern ESP8266WebServer g_server;
extern ActionNodeAppState g_state;

namespace {

OneWire g_one_wire(ACTION_NODE_PIN_DS18);
DallasTemperature g_sensors(&g_one_wire);
Ds18PollState g_ds18_poll{};

void send_json(int code, const JsonDocument &doc) {
  String body;
  serializeJson(doc, body);
  g_server.send(code, "application/json", body);
}

void send_error(int code, const char *message) {
  JsonDocument doc;
  doc["error"] = message;
  send_json(code, doc);
}

bool body_json(JsonDocument &doc) {
  if (g_server.method() != HTTP_PUT && g_server.method() != HTTP_POST) return false;
  const String &raw = g_server.arg("plain");
  if (raw.isEmpty()) return false;
  return !deserializeJson(doc, raw);
}

void handle_health() {
  JsonDocument doc;
  doc["ok"] = true;
  doc["role"] = "action_node";
  doc["mcu"] = "esp8266";
  doc["sku"] = "r2_full";
  doc["wiring_profile"] = balansun_pilot_wire_full_wiring_wire(g_state.config.wiring);
  doc["build_wiring_profile"] = balansun_pilot_wire_full_wiring_wire(action_node_build_wiring());
  send_json(200, doc);
}

void handle_public() {
  JsonDocument doc;
  doc["hostname"] = WiFi.hostname();
  doc["product"] = "Balansun action node";
  doc["role"] = "action_node";
  send_json(200, doc);
}

void handle_action_state() {
  JsonDocument doc;
  action_node_state_to_json(g_state, doc.to<JsonObject>(), static_cast<uint32_t>(millis()));
  send_json(200, doc);
}

void handle_action_command() {
  JsonDocument doc;
  if (body_json(doc)) {
    send_error(400, "invalid json");
    return;
  }
  const char *order = doc["pilot_wire_order"] | "";
  const int duration = doc["duration_sec"] | 0;
  String err;
  if (!action_node_set_command(g_state, order, duration, millis(), err)) {
    send_error(400, err.c_str());
    return;
  }
  action_node_tick(g_state, millis());
  handle_action_state();
}

void handle_action_schedule_get() {
  JsonDocument doc;
  action_node_schedule_to_json(g_state, doc.to<JsonObject>());
  send_json(200, doc);
}

void handle_action_schedule_put() {
  JsonDocument doc;
  if (body_json(doc)) {
    send_error(400, "invalid json");
    return;
  }
  String err;
  if (!action_node_schedule_from_json(g_state, doc.as<JsonObjectConst>(), err)) {
    send_error(400, err.c_str());
    return;
  }
  action_node_tick(g_state, millis());
  handle_action_schedule_get();
}

void handle_action_config_get() {
  JsonDocument doc;
  doc["wiring_profile"] = balansun_pilot_wire_full_wiring_wire(g_state.config.wiring);
  doc["build_wiring_profile"] = balansun_pilot_wire_full_wiring_wire(action_node_build_wiring());
  doc["relay_active_low"] = g_state.config.relay_active_low;
  JsonObject pins = doc["gpio"].to<JsonObject>();
  pins["full_wave"] = ACTION_NODE_PIN_FULL;
  pins["half_negative"] = ACTION_NODE_PIN_HALF_NEG;
#if BALANSUN_PILOT_WIRE_WIRING == R2_FULL_3RELAY
  pins["half_positive"] = ACTION_NODE_PIN_HALF_POS;
#endif
  pins["ds18b20"] = ACTION_NODE_PIN_DS18;
  send_json(200, doc);
}

void handle_action_config_put() {
  JsonDocument doc;
  if (body_json(doc)) {
    send_error(400, "invalid json");
    return;
  }
  const char *wire = doc["wiring_profile"] | "";
  if (wire[0]) {
    const PilotWireFullWiring next = balansun_pilot_wire_full_wiring_from_wire(wire);
    if (!action_node_wiring_matches_build(next)) {
      send_error(409, "wiring_profile mismatch with flashed build");
      return;
    }
    g_state.config.wiring = next;
  }
  if (doc["relay_active_low"].is<bool>()) {
    g_state.config.relay_active_low = doc["relay_active_low"].as<bool>();
  }
  action_node_app_save_config(g_state);
  handle_action_config_get();
}

void handle_temperature() {
  JsonDocument doc;
  if (g_state.temperature_ok) {
    doc["temperature_c"] = g_state.temperature_c;
  }
  doc["temperature_ok"] = g_state.temperature_ok;
  send_json(200, doc);
}

void handle_wifi_get() {
  JsonDocument doc;
  doc["ssid"] = g_state.wifi_ssid;
  doc["connected"] = WiFi.status() == WL_CONNECTED;
  doc["ip"] = WiFi.localIP().toString();
  send_json(200, doc);
}

void handle_wifi_put() {
  JsonDocument doc;
  if (body_json(doc)) {
    send_error(400, "invalid json");
    return;
  }
  const char *ssid = doc["ssid"] | "";
  const char *password = doc["password"] | "";
  if (!ssid[0]) {
    send_error(400, "ssid required");
    return;
  }
  std::strncpy(g_state.wifi_ssid, ssid, sizeof(g_state.wifi_ssid) - 1);
  std::strncpy(g_state.wifi_password, password, sizeof(g_state.wifi_password) - 1);
  action_node_app_save_wifi(g_state);
  WiFi.begin(g_state.wifi_ssid, g_state.wifi_password);
  handle_wifi_get();
}

void handle_relay_on() {
  const RadiatorSchedulePeriod *period =
      radiator_schedule_active_period(g_state.schedule, action_node_wall_decihours());
  if (!period || period->kind != static_cast<uint8_t>(RadiatorPeriodKind::Regulation)) {
    send_error(409, "not in regulation window");
    return;
  }
  g_state.router.surplus_on = true;
  g_state.router.regulation_window = true;
  action_node_tick(g_state, millis());
  handle_action_state();
}

void handle_relay_off() {
  const RadiatorSchedulePeriod *period =
      radiator_schedule_active_period(g_state.schedule, action_node_wall_decihours());
  if (!period || period->kind != static_cast<uint8_t>(RadiatorPeriodKind::Regulation)) {
    send_error(409, "not in regulation window");
    return;
  }
  g_state.router.surplus_on = false;
  g_state.router.regulation_window = true;
  action_node_tick(g_state, millis());
  handle_action_state();
}

}  // namespace

void action_node_http_register_routes(void) {
  g_server.on("/api/v1/health", HTTP_GET, handle_health);
  g_server.on("/api/v1/public", HTTP_GET, handle_public);
  g_server.on("/api/v1/action/state", HTTP_GET, handle_action_state);
  g_server.on("/api/v1/action/command", HTTP_PUT, handle_action_command);
  g_server.on("/api/v1/action/schedule", HTTP_GET, handle_action_schedule_get);
  g_server.on("/api/v1/action/schedule", HTTP_PUT, handle_action_schedule_put);
  g_server.on("/api/v1/action/config", HTTP_GET, handle_action_config_get);
  g_server.on("/api/v1/action/config", HTTP_PUT, handle_action_config_put);
  g_server.on("/api/v1/temperature", HTTP_GET, handle_temperature);
  g_server.on("/api/v1/wifi", HTTP_GET, handle_wifi_get);
  g_server.on("/api/v1/wifi", HTTP_PUT, handle_wifi_put);
  g_server.on("/relay/on", HTTP_GET, handle_relay_on);
  g_server.on("/relay/off", HTTP_GET, handle_relay_off);
  g_server.onNotFound([]() { g_server.send(404, "application/json", "{\"error\":\"not_found\"}"); });
}

void action_node_poll_temperature(ActionNodeAppState &state) {
  const uint32_t now = static_cast<uint32_t>(millis());
  if (!g_ds18_poll.pending) {
    g_sensors.requestTemperatures();
    ds18_poll_begin_request(g_ds18_poll, now);
    return;
  }
  if (!ds18_poll_conversion_ready(g_ds18_poll, now)) {
    return;
  }
  const float c = g_sensors.getTempCByIndex(0);
  ds18_poll_mark_read(g_ds18_poll);
  if (c == DEVICE_DISCONNECTED_C) {
    state.temperature_ok = false;
  } else {
    state.temperature_c = c;
    state.temperature_ok = true;
  }
}

void action_node_temperature_begin(void) {
  g_sensors.begin();
  g_sensors.setWaitForConversion(false);
}
