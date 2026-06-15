/*
 * Auto-split from api_v1_routes.cpp — see api_v1_common.h
 */
#include "api_v1_common.h"
#include "balansun_api_caps.h"
#include "balansun_hw_presence.h"
#include "balansun_temperature.h"
#include "metering/pmqtt_bindings.h"
void handle_get_sources_diagnostics() {
  API_AUTH_GUARD();
  API_TELEMETRY_READY_GUARD();
  BalansunJsonDoc _balansunJsonPool1 = balansun_json_doc_alloc(8192);
  JsonDocument &doc = _balansunJsonPool1;
  doc["source"] = Source;
  doc["source_data"] = Source_data;
  doc["date"] = time_sync_valid ? sync_clock_str : "";
  doc["date_valid"] = time_sync_valid;
  balansun_temperature_set_primary_c_json(doc.as<JsonObject>());
  doc["frequency_hz"] = mains_frequency_hz;
  doc["voltage_house_v"] = house_voltage_v;
  doc["current_house_a"] = house_current_a;
  doc["pf_house"] = house_power_factor;
  doc["voltage_second_v"] = second_voltage_v;
  doc["current_second_a"] = second_current_a;
  doc["pf_second"] = second_power_factor;
  int linky_tail_max = 768;
  if (server.hasArg("linky_tail")) {
    int q = server.arg("linky_tail").toInt();
    if (q > 0 && q <= 2048) {
      linky_tail_max = q;
    }
  }
  JsonObject root = doc.as<JsonObject>();
  balansun_sources_diagnostics_append_meter_payload(root, linky_tail_max);
  balansun_sources_diagnostics_append_summary(root);
  balansun_hw_presence_append_json(root);
  pmqtt_bindings_append_diagnostics(root);
  String out;
  serializeJson(doc, out);
  api_send_json(server, 200, out);
}

void handle_get_sources_brute_panel() {
  API_AUTH_GUARD();
  String out;
  balansun_sources_brute_panel_json(out);
  api_send_json(server, 200, out);
}

void handle_post_history_reset() {
  API_AUTH_GUARD();
  eepromClearConsumptionHistory();
  for (int i = 0; i < kBalansunPwrHist5mnSlots; i++) {
    tabPwHouse_5mn[i] = 0;
    tabPw_Triac_5mn[i] = 0;
    tabTemperature_5mn[i] = 0;
  }
  for (int i = 0; i < kBalansunPwrHist2sSlots; i++) {
    tabPwHouse_2s[i] = 0;
    tabPw_Triac_2s[i] = 0;
    tabPvaHouse_2s[i] = 0;
    tabPva_Triac_2s[i] = 0;
    tabTemperature_2s[i] = 0;
  }
  house_day_energy_import_wh = 0;
  house_day_energy_export_wh = 0;
  second_day_energy_import_wh = 0;
  second_day_energy_export_wh = 0;
  api_send_json(server, 200, "{\"ok\":true,\"message\":\"history reset\"}");
}

void handle_post_mqtt_discover() {
  API_AUTH_GUARD();
  ApiMqttRepublishDiscovery();
  api_send_json(server, 200, "{\"ok\":true,\"action\":\"discovery_republished\"}");
}

void handle_post_mqtt_reconnect() {
  API_AUTH_GUARD();
  ApiMqttReconnect();
  api_send_json(server, 200, "{\"ok\":true,\"action\":\"reconnect_scheduled\"}");
}

void handle_post_mqtt_publish_now() {
  API_AUTH_GUARD();
  ApiMqttPublishNow();
  api_send_json(server, 200, "{\"ok\":true,\"action\":\"publish_now\"}");
}

void handle_post_mqtt_test() {
  API_AUTH_GUARD();
  String host = ip32ToDotted(MQTTIP);
  uint16_t port = (uint16_t)MQTTPort;
  String user = MQTTUser.c_str();
  String pwd = MQTTPwd.c_str();
  String deviceName = MQTTdeviceName.c_str();
  String body;
  if (api_http_take_plain_body(body) || (server.hasArg("plain") && server.arg("plain").length() > 0)) {
    if (body.length() == 0) body = server.arg("plain");
    if (body.length() > 384) {
      api_error(server, 413, "payload_too_large", "body exceeds limit");
      return;
    }
    JsonDocument doc;
    if (deserializeJson(doc, body)) {
      api_error(server, 400, "validation", "invalid json");
      return;
    }
    if (!doc["mqtt_ip"].isNull()) {
      const char *s = doc["mqtt_ip"];
      if (!s || !s[0]) {
        api_error(server, 400, "validation", "bad ip for mqtt_ip");
        return;
      }
      host = String(s);
    }
    if (!doc["mqtt_port"].isNull()) port = (uint16_t)(int)doc["mqtt_port"];
    if (!doc["mqtt_user"].isNull()) user = doc["mqtt_user"].as<String>();
    if (!doc["mqtt_password"].isNull()) pwd = doc["mqtt_password"].as<String>();
    if (!doc["mqtt_device_name"].isNull()) deviceName = doc["mqtt_device_name"].as<String>();
  }
  balansun_apply_default_mqtt_device_name(deviceName);
  int errCode = 0;
  String msg;
  const bool connected = ApiMqttTestConnection(host, port, user, pwd, deviceName, errCode, msg);
  JsonDocument out;
  out["ok"] = connected;
  out["mqtt_connected"] = connected;
  out["error_code"] = errCode;
  out["message"] = msg;
  String json;
  serializeJson(out, json);
  api_send_json(server, connected ? 200 : 502, json);
}

void handle_post_pmqtt_preview() {
  API_AUTH_GUARD();
  BalansunJsonDoc _balansunJsonPool2 = balansun_json_doc_alloc(4096);
  JsonDocument &doc = _balansunJsonPool2;
  if (!api_deserialize_json_body(server, doc, 4096, false)) return;
  if (doc["pmqtt_bindings"].isNull() || !doc["pmqtt_bindings"].is<JsonArray>()) {
    api_error(server, 400, "validation", "pmqtt_bindings array required");
    return;
  }
  BalansunJsonDoc _balansunJsonPool3 = balansun_json_doc_alloc(4096);
  JsonDocument &out = _balansunJsonPool3;
  JsonArray results = out["results"].to<JsonArray>();
  String err;
  if (!pmqtt_bindings_preview(doc["pmqtt_bindings"].as<JsonArray>(), results, &err)) {
    api_error(server, 400, "validation", err.c_str());
    return;
  }
  out["ok"] = true;
  String json;
  serializeJson(out, json);
  api_send_json(server, 200, json);
}

#if defined(BALANSUN_ENABLE_SOURCE_TEST_API)
void handle_post_sources_test_inject() {
  API_AUTH_GUARD();
  API_SAFETY_LOCKOUT_GUARD();
  API_CAP_GUARD(BalansunCap::SourceTestInject);
  constexpr size_t kInjectBodyMax = 2048;
  const size_t storedLen = api_http_plain_body_length();
  const char *stored = api_http_plain_body_data();
  const char *validatePtr = stored;
  size_t validateLen = storedLen;
  if (validateLen == 0 && server.hasArg("plain") && server.arg("plain").length() > 0) {
    validatePtr = server.arg("plain").c_str();
    validateLen = server.arg("plain").length();
  }
  if (validateLen > kInjectBodyMax) {
    api_http_clear_plain_body();
    api_error(server, 413, "payload_too_large", "body exceeds limit");
    return;
  }
  if (validateLen > 0 && validatePtr != nullptr) {
    std::string errStd;
    if (!balansun_meter_json_logic_validate_inject(validatePtr, &errStd)) {
      api_http_clear_plain_body();
      api_error(server, 400, "validation", errStd.c_str());
      return;
    }
  }
  BalansunJsonDoc _balansunJsonPool4 = balansun_json_doc_alloc(2048);
  JsonDocument &doc = _balansunJsonPool4;
  if (!api_deserialize_json_body(server, doc, 2048, false)) return;
  if (!doc["sim"].isNull() && doc["sim"].is<JsonObject>()) {
    JsonObject sim = doc["sim"].as<JsonObject>();
    if (!sim["wall_decihours"].isNull()) {
      wall_clock_decihours = (int)sim["wall_decihours"].as<long>();
    }
    if (!sim["temperature_c"].isNull()) {
      temperature = sim["temperature_c"].as<float>();
    }
  }
  String err;
  if (!ApplyMeterSnapshotFromJson(doc.as<JsonObject>(), &err)) {
    api_error(server, 400, "validation", err.c_str());
    return;
  }
  api_send_json(server, 200, "{\"ok\":true}");
}
#endif

