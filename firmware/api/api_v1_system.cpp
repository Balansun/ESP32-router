/*
 * Auto-split from api_v1_routes.cpp — see api_v1_common.h
 */
#include "api_v1_common.h"
#include "balansun_api_caps.h"
#include "balansun_config_cap_logic.h"
#include "api_access_token.h"
#include "app_wifi_setup.h"
#include "captive_dns.h"
#include "balansun_config_store.h"
#include "balansun_forward.h"
#include "balansun_pmqtt_bindings_nvs.h"
#include "balansun_self_test.h"
#include "balansun_pin_map.h"
#include "balansun_pin_map_apply.h"
#include "balansun_product_profile.h"
#include "balansun_persistence.h"
static void api_wifi_append_public_fields(JsonObject wifi) {
  const bool setupAp = balansun_wifi_soft_ap_setup_active();
  wifi["setup_ap"] = setupAp;
  if (setupAp) {
    wifi["mode"] = "ap";
    wifi["connected"] = false;
    return;
  }
  bool staMode = false;
  bool wifiConnected = false;
  api_wifi_public_status(staMode, wifiConnected);
  wifi["mode"] = staMode ? "sta" : "ap";
  wifi["connected"] = wifiConnected;
}

void handle_get_wifi() {
  API_AUTH_GUARD();
  JsonDocument doc;
  doc["ssid"] = ssid;
  doc["password"] = password;
  const bool setupAp = balansun_wifi_soft_ap_setup_active();
  doc["setup_ap"] = setupAp;
  if (setupAp) {
    doc["mode"] = "ap";
    doc["connected"] = false;
    doc["rssi"] = WiFi.RSSI();
    doc["ip"] = WiFi.softAPIP().toString();
  } else {
    bool staMode = false;
    bool wifiConnected = false;
    api_wifi_public_status(staMode, wifiConnected);
    doc["mode"] = staMode ? "sta" : "ap";
    doc["connected"] = wifiConnected;
    doc["rssi"] = WiFi.RSSI();
    doc["ip"] = WiFi.localIP().toString();
  }
  String out;
  serializeJson(doc, out);
  api_send_json(server, 200, out);
}

void handle_put_wifi() {
  API_AUTH_GUARD();
  String body;
  if (!api_require_json_body(server, body, kWifiBodyMax, false)) {
    Serial.println("WIFI_PUT: rejected request (missing/invalid JSON body)");
    return;
  }
  Serial.printf(
      "WIFI_PUT: body_len=%u args=%d content_type=%s\n",
      static_cast<unsigned>(body.length()),
      server.args(),
      server.header("Content-Type").c_str());
  for (int i = 0; i < server.args(); i++) {
    const String k = server.argName(i);
    const String v = server.arg(i);
    Serial.printf("WIFI_PUT: arg[%d] %s len=%u\n", i, k.c_str(), static_cast<unsigned>(v.length()));
  }
  String ssidValue;
  String passwordValue;
  bool persist = true;
  bool parsedFromJson = false;
  if (body.length() > 0) {
    JsonDocument doc;
    DeserializationError e = deserializeJson(doc, body);
    if (e) {
      Serial.printf("WIFI_PUT: json_parse error=%s\n", e.c_str());
      api_error(server, 400, "json_parse", e.c_str());
      return;
    }
    ssidValue = String(doc["ssid"] | "");
    passwordValue = String(doc["password"] | "");
    persist = doc["persist"] | true;
    parsedFromJson = true;
  } else {
    // Fallback for captive-portal/proxy clients that submit URL/form args.
    if (server.hasArg("ssid")) ssidValue = server.arg("ssid");
    if (server.hasArg("password")) passwordValue = server.arg("password");
    if (server.hasArg("persist")) {
      const String rawPersist = server.arg("persist");
      persist = !(rawPersist == "0" || rawPersist == "false" || rawPersist == "off");
    }
    Serial.printf(
        "WIFI_PUT: empty JSON body; fallback args ssid=%s password=%s persist=%s\n",
        ssidValue.length() > 0 ? "yes" : "no",
        passwordValue.length() > 0 ? "yes" : "no",
        persist ? "true" : "false");
  }
  if (!parsedFromJson && ssidValue.length() == 0) {
    Serial.println("WIFI_PUT: missing payload in both JSON body and args");
    api_error(server, 400, "json_parse", "EmptyInput");
    return;
  }
  const char *newSsid = ssidValue.c_str();
  const char *newPassword = passwordValue.c_str();
  if (strlen(newSsid) == 0) {
    Serial.println("WIFI_PUT: validation error ssid required");
    api_error(server, 400, "validation", "ssid required");
    return;
  }
  Serial.printf("WIFI_PUT: ssid=\"%s\" pass_len=%u\n", newSsid, static_cast<unsigned>(strlen(newPassword)));
  ssid = String(newSsid);
  password = String(newPassword);
  int addr = persist ? persistConfigToEeprom() : 0;
  Serial.printf("WIFI_PUT: persist=%s eeprom_bytes=%d\n", persist ? "true" : "false", addr);
  if (persist && addr < 0) {
    api_error(server, 500, "eeprom_error", "EEPROM commit failed; Wi-Fi not saved to flash");
    return;
  }
  if (persist && !balansun_wifi_save_sta_profile()) {
    Serial.println(F("WIFI_PUT: NVS profile save failed (EEPROM ok)"));
  }
  JsonDocument outDoc;
  outDoc["ok"] = true;
  outDoc["persisted"] = persist;
  outDoc["eeprom_bytes"] = addr;
  outDoc["reconnect_started"] = true;
  outDoc["reboot_scheduled"] = persist;
  String out;
  serializeJson(outDoc, out);
  api_send_json(server, 200, out);
  if (persist) {
    Serial.println("WIFI_PUT: scheduling reboot in 2000ms");
    RequestReboot(2000);
  } else {
    Serial.println("WIFI_PUT: reconnecting in STA mode without reboot");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.begin(ssid.c_str(), password.c_str());
  }
}

/** Scan needs the STA radio; pure WIFI_AP cannot see surrounding networks. */
void wifi_ensure_scan_capable() {
  if (WiFi.getMode() == WIFI_AP) {
    WiFi.mode(WIFI_AP_STA);
    delay(100);
  }
  if (WiFi.getMode() != WIFI_STA && ap_default_ssid != nullptr && WiFi.softAPIP() == IPAddress(0, 0, 0, 0)) {
    const char *psk = (ap_default_psk != nullptr && ap_default_psk[0] != '\0') ? ap_default_psk : nullptr;
    WiFi.softAP(ap_default_ssid, psk, 6, 0, 4);
    delay(100);
  }
}

void wifi_send_scan_json(int n, bool scanError) {
  BalansunJsonDoc _balansunJsonPool1 = balansun_json_doc_alloc(2048);
  JsonDocument &doc = _balansunJsonPool1;
  doc["scanning"] = false;
  if (scanError) {
    doc["scan_error"] = true;
  }
  JsonArray networks = doc["networks"].to<JsonArray>();
  for (int i = 0; i < n; i++) {
    JsonObject o = networks.add<JsonObject>();
    o["ssid"] = WiFi.SSID(i);
    o["rssi"] = WiFi.RSSI(i);
    o["channel"] = WiFi.channel(i);
    o["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
  }
  String out;
  serializeJson(doc, out);
  api_send_json(server, 200, out);
  WiFi.scanDelete();
  if (balansun_wifi_soft_ap_setup_active()) {
    balansun_captive_dns_start(WiFi.softAPIP());
  }
  balansun_http_ensure_listening();
}

void wifi_start_async_scan() {
  WiFi.scanDelete();
  WiFi.scanNetworks(true);
}

void handle_get_wifi_scan() {
  API_AUTH_GUARD();
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
  wifi_ensure_scan_capable();
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) {
    api_send_json(server, 202, "{\"scanning\":true,\"networks\":[]}");
    return;
  }
  if (n < 0) {
    wifi_start_async_scan();
    api_send_json(server, 202, "{\"scanning\":true,\"networks\":[]}");
    return;
  }
  wifi_send_scan_json(n, false);
}

void handle_post_factory_reset() {
  API_AUTH_GUARD();
  api_session_clear();
  balansun_config_store_wipe();
  balansun_pmqtt_bindings_nvs_clear();
  balansun_wifi_clear_sta_profile();
  eepromClearConsumptionHistory();
  balansun_config_apply_factory_defaults();
  balansun_self_test_set_pending(true);
  balansun_eeprom_config_loaded = true;
  persistConfigToEeprom();
  api_send_json(server, 200, "{\"ok\":true,\"message\":\"factory reset scheduled\"}");
  RequestReboot(500);
}

void handle_post_save_now() {
  API_AUTH_GUARD();
  int addr = persistConfigToEeprom();
  JsonDocument doc;
  doc["ok"] = true;
  doc["eeprom_bytes"] = addr;
  String out;
  serializeJson(doc, out);
  api_send_json(server, 200, out);
}

void handle_get_eeprom() {
  API_AUTH_GUARD();
  JsonDocument doc;
  doc["used_percent"] = P_cent_EEPROM;
  doc["total_bytes"] = kEepromSize;
  doc["used_bytes"] = RomUsedBytes;
  JsonObject addr = doc["key_addresses"].to<JsonObject>();
  addr["parameters"] = kAdrParaActions;
  addr["history"] = 0;
  String out;
  serializeJson(doc, out);
  api_send_json(server, 200, out);
}

void handle_get_time() {
  API_AUTH_GUARD();
  JsonDocument doc;
  doc["tz"] = TimeTz;
  doc["ntp1"] = TimeNtp1;
  doc["ntp2"] = TimeNtp2;
  doc["date_valid"] = time_sync_valid;
  doc["now"] = sync_clock_str;
  String out;
  serializeJson(doc, out);
  api_send_json(server, 200, out);
}

void handle_get_system_arduino_ota() {
  API_AUTH_GUARD();
  JsonDocument doc;
  doc["password_set"] = arduinoOtaPassword.length() > 0;
  String out;
  serializeJson(doc, out);
  api_send_json(server, 200, out);
}

/** Non-sensitive UI bootstrap — always readable (see api_auth_public_read). */
void handle_get_public() {
  JsonDocument doc;
  JsonObject auth = doc["http_auth"].to<JsonObject>();
  auth["enabled"] = api_auth_enabled();
  auth["username"] = "admin";
  JsonObject device = doc["device"].to<JsonObject>();
  device["router_name"] = routerName;
  device["firmware_version"] = Version;
  device["source_configured"] = (Source.length() > 0 && Source != "NotDef");
  JsonObject wifi = doc["wifi"].to<JsonObject>();
  api_wifi_append_public_fields(wifi);
  String out;
  serializeJson(doc, out);
  api_send_json(server, 200, out);
}

void handle_put_system_http_auth() {
  API_AUTH_GUARD();
  JsonDocument doc;
  if (!api_deserialize_json_body(server, doc, kHttpAuthBodyMax, false)) return;
  if (doc["password"].isNull()) {
    api_error(server, 400, "bad_request", "JSON must contain \"password\" (string, empty to clear)");
    return;
  }
  String pw = doc["password"].as<String>();
  String err;
  if (!api_validate_password_ascii(pw, err)) {
    api_error(server, 400, "validation", err.c_str());
    return;
  }
  httpApiPassword = pw;
  api_session_clear();
  api_access_tokens_clear();
  const int addr = persistConfigToEeprom();
  if ((pw.length() > 0) != api_auth_enabled()) {
    api_error(server, 500, "eeprom_error", "failed to persist HTTP API password");
    return;
  }
  JsonDocument out;
  out["ok"] = true;
  out["enabled"] = api_auth_enabled();
  out["eeprom_bytes"] = addr;
  String s;
  serializeJson(out, s);
  api_send_json(server, 200, s);
}

bool validate_arduino_ota_password(const String &s, String &err) {
  if (s.length() > 64) {
    err = "password max length is 64";
    return false;
  }
  for (unsigned i = 0; i < s.length(); i++) {
    unsigned char c = (unsigned char)s[i];
    if (c < 32 || c > 126) {
      err = "password must be printable ASCII";
      return false;
    }
  }
  return true;
}

void handle_put_system_arduino_ota() {
  API_AUTH_GUARD();
  JsonDocument doc;
  if (!api_deserialize_json_body(server, doc, kArduinoOtaBodyMax, false)) return;
  if (doc["password"].isNull()) {
    api_error(server, 400, "bad_request", "JSON must contain \"password\" (string, empty to clear)");
    return;
  }
  String pw = doc["password"].as<String>();
  String err;
  if (!validate_arduino_ota_password(pw, err)) {
    api_error(server, 400, "validation", err.c_str());
    return;
  }
  arduinoOtaPassword = pw;
  int addr = persistConfigToEeprom();
  JsonDocument out;
  out["ok"] = true;
  out["eeprom_bytes"] = addr;
  out["message"] = "rebooting";
  String s;
  serializeJson(out, s);
  api_send_json(server, 200, s);
  RequestReboot(500);
}

void handle_post_auth_login() {
  JsonDocument doc;
  if (!api_deserialize_json_body(server, doc, kHttpAuthBodyMax, false)) return;
  if (doc["password"].isNull()) {
    api_error(server, 400, "bad_request", "JSON must contain \"password\"");
    return;
  }
  const String pw = doc["password"].as<String>();
  if (!api_auth_enabled()) {
    api_send_json(server, 200, "{\"ok\":true}");
    return;
  }
  if (!api_session_validate_password(pw)) {
    api_error(server, 401, "unauthorized", "Invalid password");
    return;
  }
  String token;
  if (!api_session_issue(token)) {
    api_error(server, 500, "internal_error", "failed to create session");
    return;
  }
  JsonDocument out;
  out["ok"] = true;
  out["token"] = token;
  String s;
  serializeJson(out, s);
  api_send_json(server, 200, s);
}

void handle_post_auth_logout() {
  API_AUTH_GUARD();
  api_session_clear();
  api_send_json(server, 200, "{\"ok\":true}");
}

void handle_put_time() {
  API_AUTH_GUARD();
  JsonDocument doc;
  if (!api_deserialize_json_body(server, doc, kTimeBodyMax, true)) return;
  if (!doc["tz"].isNull()) TimeTz = doc["tz"].as<String>();
  if (!doc["ntp1"].isNull()) TimeNtp1 = doc["ntp1"].as<String>();
  if (!doc["ntp2"].isNull()) TimeNtp2 = doc["ntp2"].as<String>();
  configTzTime(TimeTz.c_str(), TimeNtp1.c_str(), TimeNtp2.c_str());
  api_send_json(server, 200, "{\"ok\":true}");
}

void handle_post_pins_reset() {
  API_AUTH_GUARD();
  balansun_pins_reset_stored_to_defaults();
  int addr = persistConfigToEeprom();
  if (addr < 0) {
    api_error(server, 500, "eeprom_error", "EEPROM commit failed");
    return;
  }
  pinMapRebootPending = true;
  JsonDocument out;
  out["ok"] = true;
  out["reboot_required"] = true;
  String s;
  serializeJson(out, s);
  api_send_json(server, 200, s);
}

void handle_get_system_pins() {
  API_AUTH_GUARD();
  JsonDocument doc;
  JsonObject pins = doc.to<JsonObject>();
  balansun_pins_append_json(pins);
  String s;
  serializeJson(doc, s);
  api_send_json(server, 200, s);
}

void handle_put_system_pins() {
  API_AUTH_GUARD();
  constexpr size_t kPinsBodyMax = 512;
  if (api_reject_oversized_body(server, kPinsBodyMax)) return;
  BalansunJsonDoc pool = balansun_json_doc_alloc(kPinsBodyMax);
  JsonDocument &doc = pool;
  if (!api_deserialize_json_body(server, doc, kPinsBodyMax, true)) return;
  JsonObjectConst root = doc.as<JsonObjectConst>();
  const char *pinKeys[] = {"pin_triac_dim",     "pin_zero_cross", "pin_uart_rx", "pin_uart_tx",
                           "pin_temp",        "pin_analog0",    "pin_analog1", "pin_analog2"};
  bool hasPinField = false;
  for (const char *key : pinKeys) {
    if (!root[key].isNull()) {
      hasPinField = true;
      break;
    }
  }
  if (!hasPinField) {
    api_error(server, 400, "bad_request", "JSON must include at least one pin_* field");
    return;
  }
  {
    const BalansunConfigCapContext ctx = balansun_config_cap_context_default();
    for (const char *key : pinKeys) {
      if (root[key].isNull()) continue;
      char missing[64] = "";
      if (!balansun_config_key_allowed(ctx, key, nullptr, missing)) {
        api_error_capability_disabled(server, missing);
        return;
      }
    }
  }
  String err;
  if (!balansun_pins_apply_config_json_fields(root, err)) {
    api_error(server, 400, "validation", err.c_str());
    return;
  }
  int addr = persistConfigToEeprom();
  if (addr < 0) {
    api_error(server, 500, "eeprom_error", "EEPROM commit failed; settings not saved to flash");
    return;
  }
  JsonDocument out;
  out["ok"] = true;
  out["eeprom_bytes"] = addr;
  if (pinMapRebootPending) out["reboot_required"] = true;
  String s;
  serializeJson(out, s);
  api_send_json(server, 200, s);
}

void handle_post_firmware_ota_done() {
  API_AUTH_GUARD();
  if (Update.hasError()) {
    api_error(server, 500, "ota_failed", "firmware update failed");
    return;
  }
  api_send_json(server, 200, "{\"ok\":true,\"message\":\"rebooting\"}");
  RequestReboot(500);
}

void handle_firmware_ota_upload() {
  API_AUTH_GUARD();
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    persistence_flush_all();
    if (server.hasArg("md5")) Update.setMD5(server.arg("md5").c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.println(F("Firmware OTA complete — scheduling reboot"));
      RequestReboot(500);
    } else {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
  }
}

