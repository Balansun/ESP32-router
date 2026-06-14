/*
 * Diagnostics API — config audit log, commissioning self-test (IDEA-D1, D7).
 */
#include "api_v1_common.h"
#include "balansun_api_caps.h"
#include "balansun_config_audit.h"
#include "balansun_hw_presence.h"
#include "balansun_json.h"
#include "balansun_regulation_safety.h"
#include "balansun_self_test.h"
#include "balansun_self_test_safety_runtime.h"
#include "balansun_status_led.h"

void handle_get_system_audit() {
  API_AUTH_GUARD();
  JsonDocument doc;
  JsonArray entries = doc["entries"].to<JsonArray>();
  balansun_config_audit_append_json(entries, 20);
  String out;
  serializeJson(doc, out);
  api_send_json(server, 200, out);
}

void handle_post_health_self_test_run() {
  API_AUTH_GUARD();
  API_CAP_GUARD(BalansunCap::SelfTestTriac);
  balansun_self_test_start_run();
  api_send_json(server, 200, "{\"ok\":true,\"running\":true}");
}

void handle_post_health_self_test_skip() {
  API_AUTH_GUARD();
  balansun_self_test_skip();
  persistConfigToEeprom();
  api_send_json(server, 200, "{\"ok\":true,\"skipped\":true}");
}

#if defined(BALANSUN_ENABLE_SOURCE_TEST_API)
void handle_post_health_self_test_simulate() {
  API_AUTH_GUARD();
  JsonDocument doc;
  if (!api_deserialize_json_body(server, doc, 256, false)) return;
  if (!doc["zc_ok"].isNull()) g_self_test.zc_ok = doc["zc_ok"].as<bool>();
  if (!doc["triac_ok"].isNull()) g_self_test.triac_ok = doc["triac_ok"].as<bool>();
  if (!doc["source_ok"].isNull()) g_self_test.source_ok = doc["source_ok"].as<bool>();
  g_self_test.skipped = false;
  g_self_test.pending = false;
  g_self_test.run_epoch = balansun_self_test_stamp_run_epoch();
  balansun_self_test_disarm_boot_pending();
  if (balansun_self_test_safety_eval_now().lockout_active) {
    balansun_regulation_force_outputs_off();
  }
  persistConfigToEeprom();
  api_send_json(server, 200, "{\"ok\":true}");
}
#endif

void handle_post_health_hardware_recheck() {
  API_AUTH_GUARD();
  String body;
  if (!api_http_take_plain_body(body) && server.hasArg("plain")) {
    body = server.arg("plain");
  }
  if (body.length() > 128) {
    api_error(server, 413, "payload_too_large", "body exceeds limit");
    return;
  }
  JsonDocument doc;
  if (body.length() == 0 || deserializeJson(doc, body)) {
    api_error(server, 400, "json_parse", "Invalid JSON");
    return;
  }
  const char *id = doc["id"] | "";
  if (!id[0]) {
    api_error(server, 400, "validation", "Missing id");
    return;
  }
  JsonDocument out;
  JsonObject item = out["item"].to<JsonObject>();
  if (!balansun_hw_presence_recheck(id, item)) {
    api_error(server, 400, "validation", "Unknown or not applicable hardware id");
    return;
  }
  out["ok"] = true;
  String serialized;
  serializeJson(out, serialized);
  api_send_json(server, 200, serialized);
}

void handle_post_hardware_status_led_test() {
  API_AUTH_GUARD();
  String body;
  if (!api_http_take_plain_body(body) && server.hasArg("plain")) {
    body = server.arg("plain");
  }
  if (body.length() > 512) {
    api_error(server, 413, "payload_too_large", "body exceeds limit");
    return;
  }
  BalansunJsonDoc _jsonPool = balansun_json_doc_alloc(768);
  JsonDocument &doc = _jsonPool;
  if (body.length() == 0 || deserializeJson(doc, body)) {
    api_error(server, 400, "json_parse", "Invalid JSON");
    return;
  }
  if (doc.overflowed()) {
    api_error(server, 503, "json_buffer", "status LED test JSON overflow");
    return;
  }
  JsonObject root = doc.as<JsonObject>();
  const char *roleStr = root["role"] | "";
  StatusLedTestRole role;
  String err;
  std::string errStd;
  if (!balansun_status_led_logic_parse_test_role(roleStr, role, errStd)) {
    api_error(server, 400, "validation", errStd.c_str());
    return;
  }
  StatusLedConfig cfg;
  if (!balansun_status_led_build_test_config(root, cfg, err)) {
    api_error(server, 400, "validation", err.c_str());
    return;
  }
  if (cfg.mode == StatusLedMode::Off) {
    api_error(server, 400, "validation", "status_led_mode must not be off for test");
    return;
  }
  unsigned duration_ms = root["duration_ms"] | 3000U;
  if (!balansun_status_led_start_test(role, duration_ms, &cfg)) {
    api_error(server, 400, "validation", "status LED test could not start");
    return;
  }
  JsonDocument out;
  out["ok"] = true;
  out["role"] = roleStr;
  out["duration_ms"] = duration_ms > 10000 ? 10000 : (duration_ms < 500 ? 500 : duration_ms);
  JsonArray act = out["status_led_color_activity"].to<JsonArray>();
  act.add(cfg.color_activity.r);
  act.add(cfg.color_activity.g);
  act.add(cfg.color_activity.b);
  JsonArray reg = out["status_led_color_regulation"].to<JsonArray>();
  reg.add(cfg.color_regulation.r);
  reg.add(cfg.color_regulation.g);
  reg.add(cfg.color_regulation.b);
  JsonArray reboot = out["status_led_color_reboot"].to<JsonArray>();
  reboot.add(cfg.color_reboot.r);
  reboot.add(cfg.color_reboot.g);
  reboot.add(cfg.color_reboot.b);
  JsonArray ap = out["status_led_color_ap"].to<JsonArray>();
  ap.add(cfg.color_ap.r);
  ap.add(cfg.color_ap.g);
  ap.add(cfg.color_ap.b);
  String serialized;
  serializeJson(out, serialized);
  api_send_json(server, 200, serialized);
}
