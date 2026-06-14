/*
 * Auto-split from api_v1_routes.cpp — see api_v1_common.h
 */
#include "api_v1_common.h"
#include "balansun_config_audit.h"
#include "balansun_config_defaults_logic.h"
void handle_get_config() {
  API_AUTH_GUARD();
  const BalansunHwCaps caps = balansun_hw_caps_now();
  String out;
  {
    BalansunJsonDoc _balansunJsonPool1 = balansun_json_doc_alloc(caps.config_export_json_cap);
  JsonDocument &doc = _balansunJsonPool1;
    doc["schema_version"] = 5;
    JsonObject cfg = doc["config"].to<JsonObject>();
    balansun_config_append_export(cfg, !caps.full_config_export);
    balansun_config_append_export_metadata(cfg, caps);
    if (doc.overflowed()) {
      String msg = "config JSON overflow";
      if (!caps.full_config_export && Source == String("Pmqtt")) {
        msg = "config JSON overflow; pmqtt_bindings are not inlined on constrained tier";
      }
      api_error(server, 503, "json_buffer", msg.c_str());
      return;
    }
    serializeJson(doc, out);
  }
  if (caps.full_config_export) {
    api_splice_pmqtt_bindings_json(out);
  }
  if (out.length() == 0 || out.charAt(0) != '{') {
    api_error(server, 503, "json_buffer", "config export truncated");
    return;
  }
  api_send_json(server,200, out);
}

void handle_put_config() {
  API_AUTH_GUARD();
  const BalansunHwCaps caps = balansun_hw_caps_now();
  if (api_reject_oversized_body(server, caps.put_body_max)) return;
  BalansunJsonDoc _balansunJsonPool2 = balansun_json_doc_alloc(caps.put_body_max);
  JsonDocument &doc = _balansunJsonPool2;
  if (!api_deserialize_json_body(server, doc, caps.put_body_max, false)) return;
  JsonObject root = doc.as<JsonObject>();
  if (root["config"].isNull()) {
    api_error(server,400, "bad_request", "PUT expects { \"config\": { ... } }");
    return;
  }
  String err;
  if (!config_apply_from_json(root["config"].as<JsonObject>(), true, err)) {
    api_config_respond_apply_err(server, err);
    return;
  }
  balansun_config_audit_record("/api/v1/config", root["config"].as<JsonObject>());
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
  api_send_json(server,200, s);
}

void handle_patch_config() {
  API_AUTH_GUARD();
  const BalansunHwCaps caps = balansun_hw_caps_now();
  if (api_reject_oversized_body(server, caps.put_body_max)) return;
  BalansunJsonDoc _balansunJsonPool3 = balansun_json_doc_alloc(caps.put_body_max);
  JsonDocument &doc = _balansunJsonPool3;
  if (!api_deserialize_json_body(server, doc, caps.put_body_max, true)) return;
  const char *allowed[] = {"dhcp_on", "ip_fixed", "gateway", "subnet_mask", "dns", "source", "ext_peer_ip",
                           "ext_peer_port", "ext_peer_path", "ext_protocol", "enphase_user", "enphase_password",
                           "meter_channel", "enphase_serial",
                           "mqtt_repeat_sec", "mqtt_ip", "mqtt_port", "mqtt_user", "mqtt_password", "mqtt_prefix",
                           "mqtt_device_name", "router_name",                            "probe_second_name", "probe_house_name",
                           "temperature_label", "temp_gpio", "temperature_slots", "calib_u", "calib_i", "pmqtt_topic", "pmqtt_schema",
                           "pmqtt_bindings",
                           "jsy_mk333_serial_baud", "install_country", "install_country_variant", "mains_nominal_v",
                           "mains_frequency_mode", "mains_frequency_hz_manual", "triac_override_max_temp_c",
                           "http_cors_enabled", "pwm_gpio", "pwm_mode", "pwm_duty_percent", "pwm_inverted",
                           "status_led_mode", "status_led_gpio_activity", "status_led_gpio_regulation",
                           "status_led_rgb_gpio", "status_led_active_low", "status_led_color_activity",
                           "status_led_color_regulation", "status_led_color_reboot", "status_led_color_ap",
                           "tempo_rte_enabled", "expert_regulation_mode", "regulation_gain",
                           "vacation_enabled", "vacation_end_epoch",
                           "max_routed_w", "mqtt_json_commands", "triac_off_when_source_stale",
                           "triac_backoff_when_heater_idle", "load_profile", "action_daily_cap_wh", "pin_triac_dim",
                           "pin_zero_cross", "pin_uart_rx", "pin_uart_tx", "pin_temp", "pin_analog0", "pin_analog1",
                           "pin_analog2"};
  JsonObject in = doc.as<JsonObject>();
  for (JsonPair kv : in) {
    bool ok = false;
    for (unsigned i = 0; i < sizeof(allowed) / sizeof(allowed[0]); i++) {
      if (strcmp(kv.key().c_str(), allowed[i]) == 0) {
        ok = true;
        break;
      }
    }
    if (!ok) {
      api_error(server,400, "unknown_key", kv.key().c_str());
      return;
    }
  }
  String err;
  if (!config_apply_from_json(in, false, err)) {
    api_config_respond_apply_err(server, err);
    return;
  }
  balansun_config_audit_record("/api/v1/config", in);
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
  api_send_json(server,200, s);
}

