/*
 * api_v1_auth_tokens.cpp — persisted API access tokens (PATs).
 */
#include "api_v1_common.h"
#include "api_access_token.h"
#include "api_token_logic.h"

static const size_t kAuthTokenBodyMax = 256;

void handle_get_auth_tokens() {
  API_AUTH_GUARD();
  if (!api_auth_enabled()) {
    api_error(server, 400, "bad_request", "HTTP API password is not enabled");
    return;
  }
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < apiAccessTokenCount; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["id"] = apiAccessTokens[i].id;
    o["label"] = apiAccessTokens[i].label.c_str();
  }
  String out;
  serializeJson(doc, out);
  api_send_json(server, 200, out);
}

void handle_post_auth_tokens() {
  API_AUTH_GUARD();
  if (!api_auth_enabled()) {
    api_error(server, 400, "bad_request", "HTTP API password is not enabled");
    return;
  }
  String body;
  if (!api_http_take_plain_body(body) && server.hasArg("plain")) {
    body = server.arg("plain");
  }
  if (body.length() > kAuthTokenBodyMax) {
    api_error(server, 413, "payload_too_large", "body exceeds limit");
    return;
  }
  JsonDocument doc;
  std::string label;
  if (body.length() > 0) {
    if (deserializeJson(doc, body)) {
      api_error(server, 400, "json_parse", "Invalid JSON");
      return;
    }
    if (!doc["label"].isNull()) {
      label = doc["label"].as<const char *>();
    }
  }
  std::string err;
  if (!api_token_validate_label(label, err)) {
    api_error(server, 400, "validation", err.c_str());
    return;
  }
  std::string token;
  uint8_t id = 0;
  std::string labelOut;
  if (!api_access_tokens_create(label, token, id, labelOut, err)) {
    api_error(server, 400, "bad_request", err.c_str());
    return;
  }
  const int addr = persistConfigToEeprom();
  if (addr < 0) {
    api_access_tokens_revoke(id, err);
    api_error(server, 500, "eeprom_error", "failed to persist token");
    return;
  }
  JsonDocument out;
  out["ok"] = true;
  out["id"] = id;
  out["label"] = labelOut.c_str();
  out["token"] = token.c_str();
  String s;
  serializeJson(out, s);
  api_send_json(server, 200, s);
}

void handle_delete_auth_token(uint8_t id) {
  API_AUTH_GUARD();
  if (!api_auth_enabled()) {
    api_error(server, 400, "bad_request", "HTTP API password is not enabled");
    return;
  }
  std::string err;
  if (!api_access_tokens_revoke(id, err)) {
    api_error(server, 404, "not_found", err.c_str());
    return;
  }
  const int addr = persistConfigToEeprom();
  if (addr < 0) {
    api_error(server, 500, "eeprom_error", "failed to persist token revocation");
    return;
  }
  api_send_json(server, 200, "{\"ok\":true}");
}

bool Api_handle_auth_tokens_subresource() {
  const String &uri = server.uri();
  const char *prefix = "/api/v1/auth/tokens/";
  if (!uri.startsWith(prefix)) return false;
  if (server.method() != HTTP_DELETE) {
    api_error(server, 405, "method_not_allowed", "use DELETE");
    return true;
  }
  API_AUTH_GUARD_R();
  String tail = uri.substring(strlen(prefix));
  if (tail.length() == 0) {
    api_error(server, 404, "not_found", "token id required");
    return true;
  }
  for (unsigned i = 0; i < tail.length(); i++) {
    if (!isdigit((unsigned char)tail[i])) {
      api_error(server, 400, "bad_request", "invalid token id");
      return true;
    }
  }
  const int id = tail.toInt();
  if (id < 1 || id > 255) {
    api_error(server, 400, "bad_request", "invalid token id");
    return true;
  }
  handle_delete_auth_token(static_cast<uint8_t>(id));
  return true;
}
