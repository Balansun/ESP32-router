#pragma once

#include "api_http_plain_body.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>
/** Pins unsafe or meaningless to drive via REST PUT /api/v1/gpio (strict). */
bool IsRestrictedGpioWrite(int gpio);
/** Pins excluded from GET /api/v1/gpio (SPI flash bus only); all other 0–33 are readable. */
bool IsRestrictedGpioRead(int gpio);
String ip32ToDotted(uint32_t ip);
bool dottedToIp32(const char *s, uint32_t &out);
/** Lab CORS headers when `httpCorsEnabled` (GET + OPTIONS extras). */
void api_apply_cors_headers(WebServer &server);
/** OPTIONS preflight for /api/v1/* — returns true if handled (204). */
bool api_try_handle_cors_preflight(WebServer &server);
void api_send_json(WebServer &server, int code, const String &json);
/** Insert persisted pmqtt_bindings array into exported config JSON (avoids serialized() truncation). */
void api_splice_pmqtt_bindings_json(String &json);
void api_error(WebServer &server, int code, const char *err, const char *msg);
/** 503 when telemetry routes are not ready — includes device_lifecycle + missing_cap. */
void api_error_telemetry_not_ready(WebServer &server);
/** 403 when a mutating route targets a compiled-out product capability. */
void api_error_capability_disabled(WebServer &server, const char *missing_cap);
/** 403 when commissioning self-test safety lockout blocks routing mutations. */
void api_error_safety_lockout(WebServer &server);
/** True when config_apply_from_json err is a capability policy (`cap:<wire>`). */
bool api_config_err_is_capability(const String &err, String &missing_cap);
/** Send 403 capability_disabled or 400 validation from config apply err. Returns true if applied. */
bool api_config_respond_apply_err(WebServer &server, const String &err);
bool api_require_json_body(WebServer &server, String &body, size_t maxLen, bool patch);
/** True when stored/plain body length exceeds maxLen (sends 413). Call before large JsonDocument pools. */
bool api_reject_oversized_body(WebServer &server, size_t maxLen);
/** Parse JSON from external plain-body store (freed after parse) or server.arg("plain"). */
bool api_deserialize_json_body(WebServer &server, JsonDocument &doc, size_t maxLen, bool patch);

/** True when `httpApiPassword` is non-empty (full API lockdown). */bool api_auth_enabled();
/** Fill `mode` (sta|ap) and `connected` from STA vs soft-AP (not raw WiFi.getMode()). */
void api_wifi_public_status(bool &staMode, bool &connected);
/** When auth is enabled, require valid Bearer session unless AP Wi‑Fi bootstrap applies. */
bool api_require_auth(WebServer &server);
/** Printable ASCII password validation (max 64 chars). */
bool api_validate_password_ascii(const String &s, String &err);

/** Drop active login session (logout or HTTP password change). */
void api_session_clear();
/** Create a new random session token; returns false on allocation failure. */
bool api_session_issue(String &tokenOut);
/** Constant-time compare against stored HTTP API password. */
bool api_session_validate_password(const String &password);
