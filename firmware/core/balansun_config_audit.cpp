#include "balansun_config_audit.h"

#include "balansun_config_audit_keys.h"

#include <cstring>

namespace {
struct AuditEntry {
  unsigned long ts_ms = 0;
  char route[40];
  char keys[96];
};

constexpr int kAuditCap = 20;
AuditEntry g_audit[kAuditCap];
uint8_t g_audit_head = 0;
uint8_t g_audit_count = 0;

void append_key(char *buf, size_t cap, const char *key) {
  if (!key || !key[0]) return;
  size_t len = strlen(buf);
  if (len > 0 && len + 1 < cap) {
    strncat(buf, ",", cap - len - 1);
    len++;
  }
  if (len < cap) strncat(buf, key, cap - len - 1);
}
}  // namespace

void balansun_config_audit_record(const char *route, JsonObjectConst body) {
  if (!route) return;
  AuditEntry &e = g_audit[g_audit_head];
  e.ts_ms = millis();
  strncpy(e.route, route, sizeof(e.route) - 1);
  e.route[sizeof(e.route) - 1] = '\0';
  e.keys[0] = '\0';
  if (!body.isNull()) {
    for (JsonPairConst kv : body) {
      if (balansun_config_audit_is_secret_key(kv.key().c_str())) continue;
      append_key(e.keys, sizeof(e.keys), kv.key().c_str());
    }
  }
  g_audit_head = (uint8_t)((g_audit_head + 1) % kAuditCap);
  if (g_audit_count < kAuditCap) g_audit_count++;
}

void balansun_config_audit_append_json(JsonArray arr, int max_entries) {
  if (max_entries <= 0) max_entries = kAuditCap;
  const int n = g_audit_count < max_entries ? g_audit_count : max_entries;
  for (int i = 0; i < n; i++) {
    const int idx = (int)((g_audit_head + kAuditCap - 1 - i) % kAuditCap);
    const AuditEntry &e = g_audit[idx];
    JsonObject o = arr.add<JsonObject>();
    o["ts_ms"] = e.ts_ms;
    o["route"] = e.route;
    if (e.keys[0]) {
      JsonArray keys = o["keys"].to<JsonArray>();
      char buf[96];
      strncpy(buf, e.keys, sizeof(buf) - 1);
      buf[sizeof(buf) - 1] = '\0';
      char *tok = strtok(buf, ",");
      while (tok) {
        keys.add(tok);
        tok = strtok(nullptr, ",");
      }
    }
  }
}
