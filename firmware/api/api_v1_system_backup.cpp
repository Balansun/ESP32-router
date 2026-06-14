/*
 * api_v1_system_backup.cpp — device backup export/import (monolithic + sequenced parts).
 */
#include "api_v1_common.h"
#include "actions_api.h"
#include "api_access_token.h"
#include "app_wifi_setup.h"
#include "balansun_config_audit.h"
#include "balansun_config_defaults_logic.h"
#include "balansun_api_caps.h"
#include "balansun_hw_profile.h"

#include <string>

namespace {

constexpr int kBackupSchemaVersion = 1;
constexpr const char *kPartNetwork = "network";
constexpr const char *kPartConfig = "config";
constexpr const char *kPartActions = "actions";

enum class BackupPart : uint8_t { None, Network, Config, Actions };

String backup_exported_at_iso() {
  if (time_sync_valid && sync_clock_str.length() > 0) {
    return sync_clock_str.toString();
  }
  char buf[32];
  snprintf(buf, sizeof(buf), "uptime-%lu", static_cast<unsigned long>(millis() / 1000UL));
  return String(buf);
}

void backup_append_api_block(JsonObject api) {
  bool any = false;
  if (api_auth_enabled() && httpApiPassword.length() > 0) {
    api["http_api_password"] = httpApiPassword;
    any = true;
  }
  if (apiAccessTokenCount > 0) {
    JsonArray arr = api["access_tokens"].to<JsonArray>();
    for (int i = 0; i < apiAccessTokenCount; i++) {
      JsonObject o = arr.add<JsonObject>();
      o["id"] = apiAccessTokens[i].id;
      o["label"] = apiAccessTokens[i].label.c_str();
      o["token"] = apiAccessTokens[i].token_hex.c_str();
    }
    any = true;
  }
  if (!any) {
    api.remove("http_api_password");
    api.remove("access_tokens");
  }
}

bool backup_apply_api_block(JsonObject api, String &err) {
  err.clear();
  if (api.isNull()) return true;

  if (!api["http_api_password"].isNull()) {
    const char *pw = api["http_api_password"] | "";
    String pwStr(pw);
    if (!api_validate_password_ascii(pwStr, err)) return false;
    httpApiPassword = pwStr;
    api_session_clear();
  }

  if (!api["access_tokens"].isNull()) {
    if (!api["access_tokens"].is<JsonArray>()) {
      err = "api.access_tokens must be an array";
      return false;
    }
    JsonArray arr = api["access_tokens"].as<JsonArray>();
    if (arr.size() > kApiAccessTokenMax) {
      err = "too many access tokens";
      return false;
    }
    ApiAccessTokenEntry entries[kApiAccessTokenMax];
    int n = 0;
    for (JsonObject o : arr) {
      if (n >= kApiAccessTokenMax) break;
      if (o["id"].isNull() || o["token"].isNull()) {
        err = "access token entry requires id and token";
        return false;
      }
      entries[n].id = static_cast<uint8_t>(o["id"].as<int>());
      entries[n].label = o["label"] | "";
      entries[n].token_hex = o["token"].as<const char *>();
      n++;
    }
    std::string tokenErr;
    if (!api_access_tokens_replace_all(entries, n, tokenErr)) {
      err = tokenErr.c_str();
      return false;
    }
  }
  return true;
}

BackupPart backup_part_from_name(const char *name) {
  if (!name) return BackupPart::None;
  if (strcmp(name, kPartNetwork) == 0) return BackupPart::Network;
  if (strcmp(name, kPartConfig) == 0) return BackupPart::Config;
  if (strcmp(name, kPartActions) == 0) return BackupPart::Actions;
  return BackupPart::None;
}

bool backup_parse_part_uri(const String &uri, BackupPart &partOut) {
  const char *prefix = "/api/v1/system/backup/part/";
  if (!uri.startsWith(prefix)) return false;
  const String tail = uri.substring(strlen(prefix));
  if (tail.length() == 0) return false;
  partOut = backup_part_from_name(tail.c_str());
  return partOut != BackupPart::None;
}

void backup_overflow_error(const BalansunHwCaps &caps) {
  String msg = "backup JSON overflow";
  if (!caps.full_config_export && Source == String("Pmqtt")) {
    msg =
        "backup JSON overflow on constrained tier; pmqtt_bindings stay on device — export from "
        "standard/extended tier or use /system/backup/part/*";
  }
  api_error(server, 503, "json_buffer", msg.c_str());
}

void backup_build_config_section(JsonObject cfg, const BalansunHwCaps &caps) {
  balansun_config_append_export(cfg, !caps.full_config_export);
  balansun_config_append_export_metadata(cfg, caps);
}

void backup_build_actions_section(JsonObject actions, const BalansunHwCaps &caps) {
  actions["schema_version"] = API_ACTION_SCHEMA_VERSION;
  JsonArray actArr = actions["actions"].to<JsonArray>();
  if (caps.full_config_export) {
    actions["nb_actions"] = NbActions;
    api_action_append_config_array(actArr);
  } else {
    int exported = 0;
    api_action_append_sparse_config_array(actArr, exported);
    actions["nb_actions"] = exported;
  }
}

bool backup_apply_wifi_time_api(JsonObject doc, String &err, bool require_wifi) {
  if (!doc["time"].isNull()) {
    JsonObject timeObj = doc["time"].as<JsonObject>();
    if (!timeObj["tz"].isNull()) TimeTz = timeObj["tz"].as<String>();
    if (!timeObj["ntp1"].isNull()) TimeNtp1 = timeObj["ntp1"].as<String>();
    if (!timeObj["ntp2"].isNull()) TimeNtp2 = timeObj["ntp2"].as<String>();
  }
  if (!doc["wifi"].isNull()) {
    JsonObject wifiObj = doc["wifi"].as<JsonObject>();
    const char *newSsid = wifiObj["ssid"] | "";
    if (strlen(newSsid) == 0) {
      err = "wifi.ssid required";
      return false;
    }
    ssid = String(newSsid);
    password = String(wifiObj["password"] | "");
  } else if (require_wifi) {
    err = "wifi required";
    return false;
  }
  if (!doc["api"].isNull()) {
    if (!backup_apply_api_block(doc["api"].as<JsonObject>(), err)) return false;
  }
  return true;
}

}  // namespace

void handle_get_system_backup() {
  API_AUTH_GUARD();
  const BalansunHwCaps caps = balansun_hw_caps_now();
  String out;
  {
    BalansunJsonDoc _balansunJsonPool1 = balansun_json_doc_alloc(caps.config_export_json_cap);
    JsonDocument &doc = _balansunJsonPool1;
    doc["backupSchemaVersion"] = kBackupSchemaVersion;
    doc["exportedAt"] = backup_exported_at_iso();
    JsonObject device = doc["device"].to<JsonObject>();
    balansun_backup_append_device_block(device, caps, !caps.full_config_export);
    JsonObject cfg = doc["config"].to<JsonObject>();
    backup_build_config_section(cfg, caps);
    JsonObject actions = doc["actions"].to<JsonObject>();
    backup_build_actions_section(actions, caps);
    JsonObject timeObj = doc["time"].to<JsonObject>();
    balansun_backup_append_time(timeObj, !caps.full_config_export);
    JsonObject wifiObj = doc["wifi"].to<JsonObject>();
    wifiObj["ssid"] = ssid;
    wifiObj["password"] = password;
    JsonObject apiObj = doc["api"].to<JsonObject>();
    backup_append_api_block(apiObj);
    if (apiObj.size() == 0) {
      doc.remove("api");
    }
    if (!caps.full_config_export && timeObj.size() == 0) {
      doc.remove("time");
    }
    if (doc.overflowed()) {
      backup_overflow_error(caps);
      return;
    }
    serializeJson(doc, out);
  }
  api_splice_pmqtt_bindings_json(out);
  api_send_json(server, 200, out);
}

void handle_put_system_backup() {
  API_AUTH_GUARD();
  API_SAFETY_LOCKOUT_GUARD();
  const BalansunHwCaps caps = balansun_hw_caps_now();
  if (api_reject_oversized_body(server, caps.put_body_max)) return;
  String err;
  {
    BalansunJsonDoc _balansunJsonPool2 = balansun_json_doc_alloc(caps.put_body_max);
    JsonDocument &doc = _balansunJsonPool2;
    if (!api_deserialize_json_body(server, doc, caps.put_body_max, false)) return;
    const int ver = doc["backupSchemaVersion"] | 0;
    if (ver != kBackupSchemaVersion) {
      api_error(server, 400, "bad_request", "unsupported backupSchemaVersion");
      return;
    }
    const bool sparse = balansun_backup_is_sparse_import(doc.as<JsonObject>());
    if (doc["wifi"].isNull()) {
      api_error(server, 400, "bad_request", "missing required backup sections");
      return;
    }
    if (!sparse && (doc["config"].isNull() || doc["actions"].isNull() || doc["time"].isNull())) {
      api_error(server, 400, "bad_request", "missing required backup sections");
      return;
    }

    if (!doc["config"].isNull()) {
      const bool fullConfig = !sparse;
      if (!config_apply_from_json(doc["config"].as<JsonObject>(), fullConfig, err)) {
        api_error(server, 400, "validation", err.c_str());
        return;
      }
      balansun_config_audit_record("/api/v1/system/backup", doc["config"].as<JsonObject>());
    }

    if (!doc["actions"].isNull()) {
      const bool ok = sparse ? api_action_apply_collection_sparse(doc["actions"].as<JsonObject>(), err, false)
                             : api_action_apply_collection_object(doc["actions"].as<JsonObject>(), err, false);
      if (!ok) {
        api_error(server, 400, "validation", err.c_str());
        return;
      }
    }

    if (!backup_apply_wifi_time_api(doc.as<JsonObject>(), err, true)) {
      api_error(server, 400, "validation", err.c_str());
      return;
    }
  }
  const int addr = persistConfigToEeprom();
  if (addr < 0) {
    api_error(server, 500, "eeprom_error", "EEPROM commit failed; settings not saved to flash");
    return;
  }
  if (!balansun_wifi_save_sta_profile()) {
    Serial.println(F("backup: WiFi NVS profile save failed (EEPROM ok)"));
  }

  JsonDocument out;
  out["ok"] = true;
  out["eeprom_bytes"] = addr;
  String s;
  serializeJson(out, s);
  api_send_json(server, 200, s);
}

void handle_get_system_backup_manifest() {
  API_AUTH_GUARD();
  const BalansunHwCaps caps = balansun_hw_caps_now();
  if (caps.full_config_export) {
    api_error(server, 404, "not_found", "manifest only on constrained tier; use GET /system/backup");
    return;
  }
  BalansunJsonDoc pool = balansun_json_doc_alloc(2048);
  JsonDocument &doc = pool;
  doc["backupSchemaVersion"] = kBackupSchemaVersion;
  doc["export_mode"] = "sparse_parts";
  doc["exportedAt"] = backup_exported_at_iso();
  JsonArray parts = doc["parts"].to<JsonArray>();
  parts.add(kPartNetwork);
  parts.add(kPartConfig);
  parts.add(kPartActions);
  JsonObject device = doc["device"].to<JsonObject>();
  balansun_backup_append_device_block(device, caps, true);
  String out;
  serializeJson(doc, out);
  api_send_json(server, 200, out);
}

static void handle_get_backup_part(BackupPart part) {
  API_AUTH_GUARD();
  const BalansunHwCaps caps = balansun_hw_caps_now();
  if (caps.full_config_export) {
    api_error(server, 404, "not_found", "backup parts only on constrained tier");
    return;
  }
  BalansunJsonDoc pool = balansun_json_doc_alloc(caps.backup_part_json_cap);
  JsonDocument &doc = pool;
  doc["backupSchemaVersion"] = kBackupSchemaVersion;
  doc["part"] = (part == BackupPart::Network)   ? kPartNetwork
                : (part == BackupPart::Config)    ? kPartConfig
                                                  : kPartActions;
  doc["exportedAt"] = backup_exported_at_iso();

  switch (part) {
    case BackupPart::Network: {
      JsonObject wifiObj = doc["wifi"].to<JsonObject>();
      wifiObj["ssid"] = ssid;
      wifiObj["password"] = password;
      JsonObject timeObj = doc["time"].to<JsonObject>();
      balansun_backup_append_time(timeObj, true);
      JsonObject apiObj = doc["api"].to<JsonObject>();
      backup_append_api_block(apiObj);
      if (apiObj.size() == 0) doc.remove("api");
      if (timeObj.size() == 0) doc.remove("time");
      break;
    }
    case BackupPart::Config: {
      JsonObject cfg = doc["config"].to<JsonObject>();
      backup_build_config_section(cfg, caps);
      break;
    }
    case BackupPart::Actions: {
      JsonObject actions = doc["actions"].to<JsonObject>();
      backup_build_actions_section(actions, caps);
      break;
    }
    default:
      break;
  }

  if (doc.overflowed()) {
    backup_overflow_error(caps);
    return;
  }
  String out;
  serializeJson(doc, out);
  if (part == BackupPart::Config) {
    api_splice_pmqtt_bindings_json(out);
  }
  api_send_json(server, 200, out);
}

static void handle_put_backup_part(BackupPart part) {
  API_AUTH_GUARD();
  API_SAFETY_LOCKOUT_GUARD();
  const BalansunHwCaps caps = balansun_hw_caps_now();
  if (caps.full_config_export) {
    api_error(server, 404, "not_found", "backup parts only on constrained tier");
    return;
  }
  if (api_reject_oversized_body(server, caps.backup_part_json_cap)) return;
  String err;
  BalansunJsonDoc pool = balansun_json_doc_alloc(caps.backup_part_json_cap);
  JsonDocument &doc = pool;
  if (!api_deserialize_json_body(server, doc, caps.backup_part_json_cap, false)) return;
  if (doc.overflowed()) {
    backup_overflow_error(caps);
    return;
  }

  switch (part) {
    case BackupPart::Network:
      if (!backup_apply_wifi_time_api(doc.as<JsonObject>(), err, true)) {
        api_error(server, 400, "validation", err.c_str());
        return;
      }
      break;
    case BackupPart::Config:
      if (doc["config"].isNull()) {
        api_error(server, 400, "bad_request", "missing config");
        return;
      }
      if (!config_merge_from_backup_json(doc["config"].as<JsonObject>(), err)) {
        api_error(server, 400, "validation", err.c_str());
        return;
      }
      balansun_config_audit_record("/api/v1/system/backup/part/config", doc["config"].as<JsonObject>());
      break;
    case BackupPart::Actions:
      if (doc["actions"].isNull()) {
        api_error(server, 400, "bad_request", "missing actions");
        return;
      }
      if (!api_action_apply_collection_sparse(doc["actions"].as<JsonObject>(), err, false)) {
        api_error(server, 400, "validation", err.c_str());
        return;
      }
      break;
    default:
      api_error(server, 400, "bad_request", "invalid part");
      return;
  }

  const int addr = persistConfigToEeprom();
  if (addr < 0) {
    api_error(server, 500, "eeprom_error", "EEPROM commit failed; settings not saved to flash");
    return;
  }
  if (part == BackupPart::Network && !balansun_wifi_save_sta_profile()) {
    Serial.println(F("backup part network: WiFi NVS profile save failed (EEPROM ok)"));
  }

  JsonDocument out;
  out["ok"] = true;
  out["part"] = (part == BackupPart::Network)   ? kPartNetwork
              : (part == BackupPart::Config)    ? kPartConfig
                                                : kPartActions;
  out["eeprom_bytes"] = addr;
  if (part == BackupPart::Actions) {
    out["reboot_recommended"] = true;
  }
  String s;
  serializeJson(out, s);
  api_send_json(server, 200, s);
}

bool Api_handle_backup_subresource() {
  const String uri = server.uri();
  if (uri == "/api/v1/system/backup/manifest") {
    if (server.method() == HTTP_GET) {
      handle_get_system_backup_manifest();
      return true;
    }
    api_error(server, 405, "method_not_allowed", "GET only");
    return true;
  }
  BackupPart part = BackupPart::None;
  if (!backup_parse_part_uri(uri, part)) return false;
  if (server.method() == HTTP_GET) {
    handle_get_backup_part(part);
    return true;
  }
  if (server.method() == HTTP_PUT) {
    handle_put_backup_part(part);
    return true;
  }
  api_error(server, 405, "method_not_allowed", "GET or PUT only");
  return true;
}
