/*
 * Auto-split from api_v1_routes.cpp — see api_v1_common.h
 */
#include "api_v1_common.h"
#include "balansun_api_caps.h"
#include "balansun_ha_state_payload.h"
#include "balansun_psram.h"
#include "balansun_regulation_modes.h"
#include "balansun_device_lifecycle_logic.h"
#include "balansun_product_caps.h"
#include "balansun_self_test.h"
#include "balansun_state_snapshot.h"
#include "balansun_hw_presence.h"
#include "balansun_triac_isr.h"
#include "balansun_state_json.h"
#include "balansun_state_orchestrator.h"
#include "balansun_temperature.h"
#include "balansun_triac_sync_logic.h"
#include "triac_api_shim.h"
void handle_get_measurements() {
  API_AUTH_GUARD();
  API_TELEMETRY_READY_GUARD();
  BalansunJsonDoc _balansunJsonPool1 = balansun_json_doc_alloc(4096);
  JsonDocument &doc = _balansunJsonPool1;
  api_append_measurements_object(doc.to<JsonObject>());
  String out;
  serializeJson(doc, out);
  api_send_json(server,200, out);
}

void handle_get_telemetry_snapshot() {
  API_AUTH_GUARD();
  API_TELEMETRY_READY_GUARD();
  BalansunJsonDoc _balansunJsonPool2 = balansun_json_doc_alloc(1536);
  JsonDocument &doc = _balansunJsonPool2;
  balansun_append_ha_state_payload(doc.to<JsonObject>());
  String out;
  serializeJson(doc, out);
  api_send_json(server, 200, out);
}

void handle_post_triac_override() {
  API_AUTH_GUARD();
  API_SAFETY_LOCKOUT_GUARD();
  API_CAP_GUARD(BalansunCap::SurplusRegulation);
  JsonDocument body;
  if (!api_deserialize_json_body(server, body, 256, false)) return;
  const char *command = body["command"] | "";
  String applyErr;
  if (!balansun_apply_triac_command(command, applyErr)) {
    api_error(server, 400, "invalid_command", applyErr.c_str());
    return;
  }
  JsonDocument out;
  out["ok"] = true;
  String serialized;
  serializeJson(out, serialized);
  api_send_json(server, 200, serialized);
}

void handle_get_tariff_tempo() {
  API_AUTH_GUARD();
  BalansunJsonDoc _balansunJsonPool3 = balansun_json_doc_alloc(512);
  JsonDocument &doc = _balansunJsonPool3;
  tempo_rte_append_api_json(doc.to<JsonObject>());
  String out;
  serializeJson(doc, out);
  api_send_json(server, 200, out);
}

void handle_get_system() {
  API_AUTH_GUARD();
  int itIn = 0;
  int itRaw = 0;
  TriacReadAndResetCounters(itIn, itRaw);
  int T = int(millis() / 36000);
  float hoursUp = float(T) / 100.0f;
  BalansunJsonDoc _balansunJsonPool4 = balansun_json_doc_alloc(1536);
  JsonDocument &doc = _balansunJsonPool4;
  doc["uptime_hours"] = hoursUp;
  doc["wifi_rssi_dbm"] = WiFi.RSSI();
  doc["wifi_bssid"] = WiFi.BSSIDstr();
  doc["mac"] = WiFi.macAddress();
  doc["ssid"] = ssid;
  doc["ip"] = WiFi.localIP().toString();
  doc["gateway"] = WiFi.gatewayIP().toString();
  doc["subnet"] = WiFi.subnetMask().toString();
  doc["dns"] = WiFi.dnsIP().toString();
  JsonArray rmsT = doc["metering_task_ms"].to<JsonArray>();
  rmsT.add((int)metering_task_ms_min);
  rmsT.add((int)metering_task_ms_avg);
  rmsT.add((int)metering_task_ms_max);
  JsonArray loopT = doc["loop_task_ms"].to<JsonArray>();
  loopT.add((int)previousLoopMin);
  loopT.add((int)previousLoopMoy);
  loopT.add((int)previousLoopMax);
  doc["eeprom_used_percent"] = P_cent_EEPROM;
  doc["irq_half_period_raw_vs_in"] = String(itIn) + "/" + String(itRaw);
  doc["triac_itmode"] = (int)zc_sync_state;
  doc["triac_zc_synced"] = zc_sync_state > 0;
  doc["configured_frequency_hz"] = balansun_mains_effective_frequency_hz();
  String out;
  serializeJson(doc, out);
  api_send_json(server,200, out);
}

void handle_get_device() {
  API_AUTH_GUARD();
  BalansunJsonDoc _balansunJsonPool5 = balansun_json_doc_alloc(1536);
  JsonDocument &doc = _balansunJsonPool5;
  doc["source"] = Source;
  doc["source_data"] = Source_data;
  doc["router_name"] = routerName;
  doc["firmware_version"] = Version;
  doc["probe_second_name"] = probeSecondName;
  doc["probe_house_name"] = probeHouseName;
  doc["ext_peer_ip"] = ip32ToDotted(ext_peer_ip);
  doc["ext_peer_port"] = ext_peer_port;
  doc["ext_peer_path"] = ext_peer_path;
  doc["temperature_label"] = temperatureSensorName;
  doc["device_uid"] = balansun_device_uid();
  JsonObject fw = doc["firmware"].to<JsonObject>();
  fw["sketch_md5"] = ESP.getSketchMD5();
  fw["sketch_size"] = ESP.getSketchSize();
  fw["free_sketch_space"] = ESP.getFreeSketchSpace();
  fw["sdk_version"] = ESP.getSdkVersion();
  JsonObject chip = doc["chip"].to<JsonObject>();
  chip["model"] = ESP.getChipModel();
  chip["revision"] = ESP.getChipRevision();
  chip["cores"] = ESP.getChipCores();
  chip["cpu_mhz"] = ESP.getCpuFreqMHz();
  chip["flash_size"] = ESP.getFlashChipSize();
  chip["flash_mhz"] = ESP.getFlashChipSpeed();
  chip["mac"] = WiFi.macAddress();
  chip["psram_size"] = balansun_psram_size();
  chip["psram_free"] = balansun_psram_free();
  balansun_hw_profile_append_device_json(doc);
  String out;
  serializeJson(doc, out);
  api_send_json(server,200, out);
}

void handle_get_state() {
  API_AUTH_GUARD();
  API_TELEMETRY_READY_GUARD();
  balansun_state_orchestrator_tick_for_http();
  BalansunJsonDoc _balansunJsonPool6 = balansun_json_doc_alloc(4096);
  JsonDocument &doc = _balansunJsonPool6;
  JsonObject measurements = doc["measurements"].to<JsonObject>();
  api_append_measurements_object(measurements);
  JsonObject actions = doc["actions_live"].to<JsonObject>();
  actions["temperature_c"] = temperature;
  actions["source"] = Source_data;
  actions["ext_peer_ip"] = ip32ToDotted(ext_peer_ip);
  actions["ext_peer_port"] = ext_peer_port;
  actions["ext_peer_path"] = ext_peer_path;
  int nb = 0;
  for (int i = 0; i < NbActions; i++) {
    if (action_regulation_enabled(load_channels[i].Actif)) nb++;
  }
  actions["active_actions_count"] = nb;
  JsonArray slots = actions["active_slots"].to<JsonArray>();
  api_action_append_live_state(slots);
  doc["triac_open_percent"] = TriacGetOpenPercent();
  JsonObject status = doc["status"].to<JsonObject>();
  const int health = balansun_compute_source_health_score();
  status["source_health"] = health;
  status["source_stale"] = balansun_source_health_is_stale(health);
  status["regulation_active"] = TriacGetOpenPercent() > 5;
  balansun_append_operational_status_json(status);
  status["site_cap_active"] = siteCapActive;
  status["mqtt_connected"] = clientMQTT.connected();
  doc["heater_load_backoff_active"] = heaterLoadBackoffActive;
  doc["temperature_c"] = temperature;
  doc["time"] = time_sync_valid ? sync_clock_str : "";
  doc["date_valid"] = time_sync_valid;
  doc["source"] = Source_data;
  JsonArray overrides = doc["override_summary"].to<JsonArray>();
  for (int i = 0; i < NbActions; i++) {
    if (load_channels[i].OverrideState == ACTION_OVERRIDE_AUTO) continue;
    JsonObject o = overrides.add<JsonObject>();
    api_append_action_override(o, i);
  }
  String out;
  serializeJson(doc, out);
  api_send_json(server, 200, out);
}

void handle_get_health() {
  API_AUTH_GUARD();
  balansun_state_orchestrator_tick_for_http();
  JsonDocument doc;
  doc["ok"] = true;
  doc["uptime_s"] = (unsigned long)(millis() / 1000UL);
  doc["telemetry_ready"] = balansun_api_telemetry_ready();
  doc["source_ok"] = meter_reading_valid;
  doc["date_valid"] = time_sync_valid;
  doc["mqtt_connected"] = clientMQTT.connected();
  doc["free_heap"] = ESP.getFreeHeap();
  doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
  doc["pin_map_fallback_active"] = pinMapFallbackActive;
  doc["pin_map_reboot_pending"] = pinMapRebootPending;
  doc["device_lifecycle"] = balansun_device_lifecycle_wire(g_balansun_state_snapshot.lifecycle);
  doc["product_profile"] = balansun_product_caps_compile_time().profile_wire;
  balansun_append_output_suspend_json(doc.as<JsonObject>());
  balansun_append_safety_lockout_json(doc.as<JsonObject>());
  JsonObject st = doc["self_test"].to<JsonObject>();
  balansun_self_test_append_health_json(st);
  balansun_hw_presence_append_json(doc.as<JsonObject>());
  if (temperature > -100) {
    doc["temperature_c"] = temperature;
  }
  balansun_temperature_append_telemetry_json(doc.as<JsonObject>());
  String out;
  serializeJson(doc, out);
  api_send_json(server, 200, out);
}

void handle_get_sources() {
  API_AUTH_GUARD();
  JsonDocument doc;
  JsonArray supported = doc["supported"].to<JsonArray>();
  for (size_t i = 0; i < balansun_source_registry_count(); i++) {
    supported.add(balansun_source_wire_at(i));
  }
  doc["current"] = Source;
  doc["current_data"] = Source_data;
  String out;
  serializeJson(doc, out);
  api_send_json(server, 200, out);
}

void handle_get_gpio() {
  API_AUTH_GUARD();
  BalansunJsonDoc _balansunJsonPool7 = balansun_json_doc_alloc(1536);
  JsonDocument &doc = _balansunJsonPool7;
  JsonObject levels = doc["levels"].to<JsonObject>();
  for (int gpio = 0; gpio <= 33; gpio++) {
    if (IsRestrictedGpioRead(gpio)) continue;
    levels[String(gpio)] = digitalRead(gpio);
  }
  doc["updated_ms"] = (unsigned long)millis();
  String out;
  serializeJson(doc, out);
  api_send_json(server, 200, out);
}

