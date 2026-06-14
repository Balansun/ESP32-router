/*
 * api_util.cpp — API session auth, JSON helpers, restricted GPIO matrix for API routes.
 * See: /en/http-api-security/; balansun_source capability flags for per-source GPIO rules.
 */
#include "api_util.h"

#include "api_http_plain_body.h"
#include "balansun_config_cap_logic.h"
#include "balansun_api_ready.h"
#include "balansun_http_service.h"
#include "app_wifi_setup.h"
#include "api_access_token.h"
#include "api_session_logic.h"
#include "api_util_logic.h"
#include "balansun_globals.h"
#include "balansun_hw_profile.h"
#include "balansun_json.h"
#include "balansun_source.h"
#include <esp_random.h>
#include <string>
#include <ArduinoJson.h>
#include <IPAddress.h>
#include <WiFi.h>
#include <cstring>

static String g_apiSessionToken;

bool IsRestrictedGpioRead(int gpio) { return api_logic_is_restricted_gpio_read(gpio); }

bool IsRestrictedGpioWrite(int gpio) {
  if (pwmGpio >= 0 && gpio == pwmGpio) return false;
  return api_logic_is_restricted_gpio_write(gpio, balansun_cap_serial_adc_gpio_restrict());
}

String ip32ToDotted(uint32_t ip) { return String(api_logic_ip32_to_dotted(ip).c_str()); }

bool dottedToIp32(const char *s, uint32_t &out) { return api_logic_dotted_to_ip32(s, out); }

void api_apply_cors_headers(WebServer &server) {
  if (!httpCorsEnabled) return;
  server.sendHeader("Access-Control-Allow-Origin", "*");
  if (server.method() == HTTP_OPTIONS) {
    server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Authorization, Content-Type, Accept");
    server.sendHeader("Access-Control-Max-Age", "86400");
  }
}

bool api_try_handle_cors_preflight(WebServer &server) {
  const bool isOptions = server.method() == HTTP_OPTIONS;
  if (!api_logic_should_handle_cors_preflight(httpCorsEnabled, isOptions, server.uri().c_str())) {
    return false;
  }
  api_apply_cors_headers(server);
  server.sendHeader("Cache-Control", "no-store");
  server.send(204);
  return true;
}

void api_send_json(WebServer &server, int code, const String &json) {
  if (httpCorsEnabled && server.method() == HTTP_GET) {
    api_apply_cors_headers(server);
  }
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("Connection", "close");
  server.send(code, "application/json", json);
  WiFiClient client = server.client();
  balansun_http_drain_client(client, static_cast<size_t>(json.length()));
}

void api_splice_pmqtt_bindings_json(String &json) {
  if (PmqttBindingsJson.length() <= 2) return;
  if (json.indexOf(",\"pmqtt_bindings\":") >= 0) return;
  {
    BalansunJsonDoc _pool = balansun_json_doc_alloc(4096);
    JsonDocument &check = _pool;
    if (deserializeJson(check, PmqttBindingsJson.c_str(), PmqttBindingsJson.length()) != DeserializationError::Ok ||
        !check.is<JsonArray>()) {
      return;
    }
  }
  // Metadata keys only — never anchor on pmqtt_schema/topic (can appear inside binding JSON).
  int anchor = json.indexOf(",\"pmqtt_bindings_on_device\"");
  if (anchor < 0) anchor = json.indexOf(",\"pmqtt_bindings_exported\"");
  if (anchor < 0) return;
  const String ins = String(",\"pmqtt_bindings\":") + PmqttBindingsJson.c_str();
  json = json.substring(0, static_cast<unsigned>(anchor)) + ins + json.substring(static_cast<unsigned>(anchor));
}

void api_error(WebServer &server, int code, const char *err, const char *msg) {
  JsonDocument doc;
  doc["error"] = err;
  doc["message"] = msg;
  String out;
  serializeJson(doc, out);
  api_send_json(server, code, out);
}

void api_error_telemetry_not_ready(WebServer &server) {
  const char *lifecycle = balansun_api_telemetry_not_ready_lifecycle_wire();
  JsonDocument doc;
  doc["error"] = "not_ready";
  doc["message"] = lifecycle;
  doc["device_lifecycle"] = lifecycle;
  doc["missing_cap"] = "telemetry";
  String out;
  serializeJson(doc, out);
  api_send_json(server, 503, out);
}

void api_error_capability_disabled(WebServer &server, const char *missing_cap) {
  JsonDocument doc;
  doc["error"] = "capability_disabled";
  doc["missing_cap"] = missing_cap ? missing_cap : "unknown";
  doc["message"] = balansun_config_cap_disabled_message(missing_cap);
  String out;
  serializeJson(doc, out);
  api_send_json(server, 403, out);
}

void api_error_safety_lockout(WebServer &server) {
  JsonDocument doc;
  doc["error"] = "safety_lockout";
  doc["missing_cap"] = "safety_lockout";
  doc["message"] = balansun_config_cap_disabled_message("safety_lockout");
  String out;
  serializeJson(doc, out);
  api_send_json(server, 403, out);
}

bool api_config_err_is_capability(const String &err, String &missing_cap) {
  const String prefix = "cap:";
  if (!err.startsWith(prefix)) return false;
  missing_cap = err.substring(prefix.length());
  return missing_cap.length() > 0;
}

bool api_config_respond_apply_err(WebServer &server, const String &err) {
  if (err == "safety_lockout") {
    api_error_safety_lockout(server);
    return true;
  }
  String missing_cap;
  if (api_config_err_is_capability(err, missing_cap)) {
    api_error_capability_disabled(server, missing_cap.c_str());
    return true;
  }
  api_error(server, 400, "validation", err.c_str());
  return true;
}

static bool api_acquire_json_body(WebServer &server, String &body) {
  if (api_http_take_plain_body(body)) return body.length() > 0;
  if (server.hasArg("plain")) {
    body = server.arg("plain");
    return body.length() > 0;
  }
  return false;
}

bool api_require_json_body(WebServer &server, String &body, size_t maxLen, bool patch) {
  (void)patch;
  if (!api_acquire_json_body(server, body)) {
    api_error(server, 400, "bad_request", "JSON body required (Content-Type: application/json)");
    return false;
  }
  if (body.length() > maxLen) {
    api_error(server, 413, "payload_too_large", "body exceeds limit");
    return false;
  }
  return true;
}

static size_t api_plain_body_length(WebServer &server) {
  const size_t storedLen = api_http_plain_body_length();
  if (storedLen > 0) return storedLen;
  if (server.hasArg("plain") && server.arg("plain").length() > 0) {
    return server.arg("plain").length();
  }
  if (server.hasHeader("Content-Length")) {
    const int contentLen = server.header("Content-Length").toInt();
    if (contentLen > 0) return static_cast<size_t>(contentLen);
  }
  return 0;
}

bool api_reject_oversized_body(WebServer &server, size_t maxLen) {
  const size_t len = api_plain_body_length(server);
  if (len <= maxLen) return false;
  api_http_clear_plain_body();
  api_error(server, 413, "payload_too_large", "body exceeds limit");
  return true;
}

bool api_deserialize_json_body(WebServer &server, JsonDocument &doc, size_t maxLen, bool patch) {
  (void)patch;
  const size_t storedLen = api_http_plain_body_length();
  if (storedLen > 0) {
    const char *stored = api_http_plain_body_data();
    if (stored == nullptr) {
      api_error(server, 400, "bad_request", "JSON body required (Content-Type: application/json)");
      return false;
    }
    if (storedLen > maxLen) {
      api_http_clear_plain_body();
      api_error(server, 413, "payload_too_large", "body exceeds limit");
      return false;
    }
    const DeserializationError e = deserializeJson(doc, stored, storedLen);
    api_http_clear_plain_body();
    if (e) {
      api_error(server, 400, "json_parse", e.c_str());
      return false;
    }
    return true;
  }
  if (!server.hasArg("plain") || server.arg("plain").length() == 0) {
    api_error(server, 400, "bad_request", "JSON body required (Content-Type: application/json)");
    return false;
  }
  const String &body = server.arg("plain");
  if (body.length() > maxLen) {
    api_error(server, 413, "payload_too_large", "body exceeds limit");
    return false;
  }
  const DeserializationError e = deserializeJson(doc, body);
  if (e) {
    api_error(server, 400, "json_parse", e.c_str());
    return false;
  }
  return true;
}

bool api_auth_enabled() { return httpApiPassword.length() > 0; }

void api_wifi_public_status(bool &staMode, bool &connected) {
  const bool staUp =
      WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0);
  if (staUp) {
    staMode = true;
    connected = true;
    return;
  }
  staMode = false;
  connected = false;
  if (WiFi.getMode() != WIFI_STA && WiFi.softAPIP() != IPAddress(0, 0, 0, 0)) {
    staMode = false;
    connected = false;
  }
}

bool api_validate_password_ascii(const String &s, String &err) {
  std::string errStd;
  const bool ok = api_logic_validate_password_ascii(std::string(s.c_str()), errStd);
  if (!ok) err = String(errStd.c_str());
  return ok;
}

void api_session_clear() { g_apiSessionToken = ""; }

static String api_session_generate_token() {
  uint8_t bytes[32];
  esp_fill_random(bytes, sizeof(bytes));
  static const char hex[] = "0123456789abcdef";
  char out[65];
  for (size_t i = 0; i < sizeof(bytes); i++) {
    out[i * 2] = hex[(bytes[i] >> 4) & 0x0f];
    out[i * 2 + 1] = hex[bytes[i] & 0x0f];
  }
  out[64] = '\0';
  return String(out);
}

bool api_session_issue(String &tokenOut) {
  tokenOut = api_session_generate_token();
  g_apiSessionToken = tokenOut;
  return true;
}

bool api_session_validate_password(const String &password) {
  return api_logic_password_constant_time_eq(std::string(password.c_str()),
                                             std::string(httpApiPassword.c_str()));
}

static bool api_session_header_valid(const String &header) {
  if (g_apiSessionToken.length() == 0) return false;
  std::string token;
  if (!api_logic_parse_bearer_token(std::string(header.c_str()), token)) return false;
  return api_logic_session_token_constant_time_eq(token, std::string(g_apiSessionToken.c_str()));
}

/** AP setup: allow clearing HTTP API password without knowing the old one. */
static bool api_auth_ap_clear_http_password(WebServer &server) {
  if (server.method() != HTTP_PUT) return false;
  if (server.uri() != "/api/v1/system/http-auth") return false;
  if (!server.hasArg("plain") && api_http_plain_body_length() == 0) return false;
  String body;
  if (!api_http_take_plain_body(body) && server.hasArg("plain")) {
    body = server.arg("plain");
  }
  JsonDocument doc;
  if (deserializeJson(doc, body)) return false;
  if (doc["password"].isNull()) return false;
  const char *pw = doc["password"] | "";
  return pw[0] == '\0';
}

/** UI bootstrap reads — no secrets; always open when API auth is enabled. */
static bool api_auth_public_read(WebServer &server) {
  if (server.method() != HTTP_GET) return false;
  const String &uri = server.uri();
  return uri == "/api/v1/public";
}

static bool api_auth_login_route(WebServer &server) {
  return server.method() == HTTP_POST && server.uri() == "/api/v1/auth/login";
}

/** AP / first-run setup: restore routes stay open without session (soft-AP only). */
static bool api_auth_ap_setup_bootstrap(WebServer &server) {
  if (!balansun_wifi_soft_ap_setup_active()) return false;
  const String &uri = server.uri();
  if (uri == "/api/v1/wifi") return true;
  if (uri.startsWith("/api/v1/wifi/")) return true;
  if (uri == "/api/v1/config" || uri == "/api/v1/actions/config" || uri == "/api/v1/time") {
    return true;
  }
  if (uri == "/api/v1/system/backup" && (server.method() == HTTP_GET || server.method() == HTTP_PUT)) {
    return true;
  }
  if (api_auth_ap_clear_http_password(server)) return true;
  if (uri == "/api/v1/system/http-auth" && server.method() == HTTP_PUT && !api_auth_enabled()) {
    return true;
  }
  if (server.method() == HTTP_POST && uri == "/api/v1/system/factory-reset") return true;
  if (server.method() == HTTP_POST && uri == "/api/v1/system/reboot") return true;
  if (server.method() == HTTP_GET && uri == "/api/v1/system/pins") return true;
  if (server.method() == HTTP_PUT && uri == "/api/v1/system/pins") return true;
  if (server.method() == HTTP_POST && uri == "/api/v1/system/pins/reset") return true;
  return false;
}

static void api_send_unauthorized(WebServer &server) {
  api_error(server, 401, "unauthorized", "Valid API session required");
}

bool api_require_auth(WebServer &server) {
  if (!api_auth_enabled()) return true;
  if (api_logic_cors_preflight_bypasses_auth(httpCorsEnabled, server.method() == HTTP_OPTIONS,
                                             server.uri().c_str())) {
    return true;
  }
  if (api_auth_public_read(server)) return true;
  if (api_auth_login_route(server)) return true;
  if (api_auth_ap_setup_bootstrap(server)) return true;
  if (!server.hasHeader("Authorization")) {
    api_send_unauthorized(server);
    return false;
  }
  const String authHeader = server.header("Authorization");
  if (api_session_header_valid(authHeader)) return true;
  std::string token;
  if (api_logic_parse_bearer_token(std::string(authHeader.c_str()), token) &&
      api_access_tokens_verify_bearer(token)) {
    return true;
  }
  api_send_unauthorized(server);
  return false;
}
