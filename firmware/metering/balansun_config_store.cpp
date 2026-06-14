#include "balansun_config_store.h"

#include <Arduino.h>
#include <Preferences.h>

#include "balansun_config_store_logic.h"

#include <cstring>

/*
 * balansun_config_store.cpp — Arduino glue for the per-domain NVS config store.
 *
 * Namespace "balansun_cfg" holds one key per domain ("hist"/"anchor"/"params"). The legacy
 * monolithic image still exists under the Arduino EEPROM namespace ("eeprom"); we read it
 * once via the image EEPROM.begin() already loaded, then mirror into domains. We never call
 * EEPROM.commit() again, so the legacy blob is frozen (no further wear) after migration.
 */

namespace {
constexpr const char *kCfgNamespace = "balansun_cfg";

uint32_t g_lastCrc[kBalansunConfigDomainCount] = {0, 0, 0};
bool g_lastCrcValid = false;
bool g_migrated = false;
int g_lastWritten = 0;

bool all_domains_present(Preferences &prefs) {
  const ConfigDomain *d = balansun_config_domains();
  for (int i = 0; i < kBalansunConfigDomainCount; i++) {
    if (prefs.getBytesLength(d[i].key) != static_cast<size_t>(d[i].len)) return false;
  }
  return true;
}

bool domain_present(Preferences &prefs, const ConfigDomain &domain) {
  return prefs.getBytesLength(domain.key) == static_cast<size_t>(domain.len);
}

void domain_seed_nvs(Preferences &prefs, const uint8_t *image, const ConfigDomain &domain, int index) {
  prefs.putBytes(domain.key, image + domain.start, domain.len);
  g_lastCrc[index] = balansun_config_crc32(image + domain.start, domain.len);
}

void domain_load_into_image(Preferences &prefs, uint8_t *image, const ConfigDomain &domain, int index) {
  prefs.getBytes(domain.key, image + domain.start, domain.len);
  g_lastCrc[index] = balansun_config_crc32(image + domain.start, domain.len);
}

void erase_legacy_eeprom_nvs_namespace() {
  Preferences legacy;
  if (legacy.begin("eeprom", /*readOnly=*/false)) {
    legacy.clear();
    legacy.end();
    Serial.println(F("config store: reclaimed legacy eeprom NVS namespace"));
  }
}
}  // namespace

void balansun_config_store_load(uint8_t *image, int size) {
  g_migrated = false;
  if (!image || size < kEepromSize) return;
  const ConfigDomain *d = balansun_config_domains();

  Preferences prefs;
  if (!prefs.begin(kCfgNamespace, /*readOnly=*/true)) {
    // Namespace does not exist yet → migrate from the legacy image already in `image`.
    prefs.end();
    if (prefs.begin(kCfgNamespace, /*readOnly=*/false)) {
      for (int i = 0; i < kBalansunConfigDomainCount; i++) {
        domain_seed_nvs(prefs, image, d[i], i);
      }
      prefs.end();
      g_lastCrcValid = true;
      g_migrated = true;
      Serial.println("config store: migrated legacy EEPROM blob into per-domain NVS keys");
      erase_legacy_eeprom_nvs_namespace();
    }
    return;
  }

  if (all_domains_present(prefs)) {
    for (int i = 0; i < kBalansunConfigDomainCount; i++) {
      domain_load_into_image(prefs, image, d[i], i);
    }
    prefs.end();
    g_lastCrcValid = true;
    erase_legacy_eeprom_nvs_namespace();
    return;
  }

  // Merge per domain: keep valid NVS slices (e.g. params with MQTT broker settings) instead of
  // clobbering everything from the frozen legacy flash image when hist/anchor are absent.
  prefs.end();
  if (prefs.begin(kCfgNamespace, /*readOnly=*/false)) {
    for (int i = 0; i < kBalansunConfigDomainCount; i++) {
      if (domain_present(prefs, d[i])) {
        domain_load_into_image(prefs, image, d[i], i);
      } else {
        domain_seed_nvs(prefs, image, d[i], i);
        g_migrated = true;
      }
    }
    prefs.end();
    g_lastCrcValid = true;
    if (g_migrated) {
      Serial.println("config store: merged missing NVS domains without resetting valid keys");
      erase_legacy_eeprom_nvs_namespace();
    }
  }
}

bool balansun_config_store_commit(const uint8_t *image, int size, uint8_t domain_mask) {
  g_lastWritten = 0;
  if (!image || size < kEepromSize) return false;
  const ConfigDomain *d = balansun_config_domains();

  Preferences prefs;
  if (!prefs.begin(kCfgNamespace, /*readOnly=*/false)) {
    Serial.println("config store: commit failed (NVS open)");
    return false;
  }
  bool ok = true;
  for (int i = 0; i < kBalansunConfigDomainCount; i++) {
    if ((domain_mask & (1u << i)) == 0) continue;
    const uint32_t crc = balansun_config_crc32(image + d[i].start, d[i].len);
    if (g_lastCrcValid && crc == g_lastCrc[i]) continue;  // unchanged → skip the write
    const size_t wrote = prefs.putBytes(d[i].key, image + d[i].start, d[i].len);
    if (wrote == static_cast<size_t>(d[i].len)) {
      g_lastCrc[i] = crc;
      g_lastWritten++;
    } else {
      Serial.printf("config store: commit failed domain %s (%u/%d bytes)\n", d[i].key,
                    static_cast<unsigned>(wrote), d[i].len);
      ok = false;
    }
  }
  prefs.end();
  g_lastCrcValid = true;
  return ok;
}

void balansun_config_store_mark_params_dirty(void) {
  if (!g_lastCrcValid) return;
  g_lastCrc[2] ^= 0xFFFFFFFFu;
}

void balansun_config_store_wipe(void) {
  const ConfigDomain *d = balansun_config_domains();
  Preferences prefs;
  if (prefs.begin(kCfgNamespace, /*readOnly=*/false)) {
    for (int i = 0; i < kBalansunConfigDomainCount; i++) {
      prefs.remove(d[i].key);
    }
    prefs.remove("pmqtt_bind");
    prefs.end();
  }
  erase_legacy_eeprom_nvs_namespace();
  g_lastCrcValid = false;
  memset(g_lastCrc, 0, sizeof(g_lastCrc));
}

void balansun_config_store_reclaim_legacy_nvs(void) { erase_legacy_eeprom_nvs_namespace(); }

bool balansun_config_store_domains_ready(void) {
  Preferences prefs;
  if (!prefs.begin(kCfgNamespace, /*readOnly=*/true)) return false;
  const bool ok = all_domains_present(prefs);
  prefs.end();
  return ok;
}

bool balansun_config_store_any_domain_present(void) {
  Preferences prefs;
  if (!prefs.begin(kCfgNamespace, /*readOnly=*/true)) return false;
  const ConfigDomain *d = balansun_config_domains();
  bool any = false;
  for (int i = 0; i < kBalansunConfigDomainCount; i++) {
    if (prefs.getBytesLength(d[i].key) > 0) {
      any = true;
      break;
    }
  }
  prefs.end();
  return any;
}

bool balansun_config_store_migrated() { return g_migrated; }

int balansun_config_store_last_written() { return g_lastWritten; }
