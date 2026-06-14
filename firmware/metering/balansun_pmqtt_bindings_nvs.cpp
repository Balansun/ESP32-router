#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_PMqtt

#include "balansun_pmqtt_bindings_nvs.h"

#include <Arduino.h>
#include <Preferences.h>

#include <cstring>

namespace {
constexpr const char *kNamespace = "balansun_cfg";
constexpr const char *kKey = "pmqtt_bind";
constexpr size_t kMaxLen = 4096;
}  // namespace

bool balansun_pmqtt_bindings_nvs_save(const char *json, size_t len) {
  if (!json || len == 0) {
    balansun_pmqtt_bindings_nvs_clear();
    return true;
  }
  if (len > kMaxLen) return false;
  Preferences prefs;
  if (!prefs.begin(kNamespace, /*readOnly=*/false)) return false;
  const size_t wrote = prefs.putBytes(kKey, json, len);
  prefs.end();
  return wrote == len;
}

bool balansun_pmqtt_bindings_nvs_load(char *out, size_t outCap, size_t *outLen) {
  if (outLen) *outLen = 0;
  if (!out || outCap == 0) return false;
  Preferences prefs;
  if (!prefs.begin(kNamespace, /*readOnly=*/true)) return false;
  const size_t stored = prefs.getBytesLength(kKey);
  if (stored == 0 || stored >= outCap) {
    prefs.end();
    return false;
  }
  const size_t n = prefs.getBytes(kKey, out, outCap - 1);
  prefs.end();
  if (n == 0 || n >= outCap) return false;
  out[n] = '\0';
  if (outLen) *outLen = n;
  return true;
}

void balansun_pmqtt_bindings_nvs_clear(void) {
  Preferences prefs;
  if (!prefs.begin(kNamespace, /*readOnly=*/false)) return;
  prefs.remove(kKey);
  prefs.end();
}

#else

#include "balansun_pmqtt_bindings_nvs.h"

bool balansun_pmqtt_bindings_nvs_save(const char *, size_t) { return true; }

bool balansun_pmqtt_bindings_nvs_load(char *, size_t, size_t *outLen) {
  if (outLen) *outLen = 0;
  return false;
}

void balansun_pmqtt_bindings_nvs_clear(void) {}

#endif
