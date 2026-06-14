#pragma once

#include <cstddef>
#include <cstdint>

#include "storage_eeprom_layout.h"

/*
 * balansun_config_store_logic.h — Pure (Arduino-free) domain split for the config image.
 *
 * Phase 2 wear reduction: instead of storing the whole 4090-byte image as ONE NVS blob
 * (rewritten in full on every commit — including the daily midnight tick), the image is
 * split into byte-range "domains" stored as independent NVS keys. A commit rewrites only
 * the domains whose bytes actually changed:
 *   - "hist"   [0 .. kEepromAdrTriacImportJ0)      annual energy fingerprint (daily slot churn)
 *   - "anchor" [.. kEepromAdrParaActions)          day-energy anchors + date + ring index (daily)
 *   - "params" [.. kEepromSize)                    Wi-Fi/MQTT/Source/actions/extension (config saves)
 *
 * Effect: the once-a-day energy tick rewrites ~1.5 KB (hist+anchor) instead of 4 KB, and the
 * params region is only rewritten when configuration actually changes. The serialization format
 * inside the image is unchanged, so the REST/golden contract is unaffected.
 */

struct ConfigDomain {
  const char *key;
  int start;
  int len;
};

constexpr int kBalansunConfigDomainCount = 3;

/** Domain table tiling the whole image [0, kEepromSize) with no gaps or overlaps. */
inline const ConfigDomain *balansun_config_domains() {
  static const ConfigDomain kDomains[kBalansunConfigDomainCount] = {
      {"hist", kEepromAdrHistoAn, kEepromAdrTriacImportJ0 - kEepromAdrHistoAn},
      {"anchor", kEepromAdrTriacImportJ0, kEepromAdrParaActions - kEepromAdrTriacImportJ0},
      {"params", kEepromAdrParaActions, kEepromSize - kEepromAdrParaActions},
  };
  return kDomains;
}

/** CRC32 (IEEE 802.3, reflected) used to detect per-domain changes between commits. */
inline uint32_t balansun_config_crc32(const uint8_t *p, int len) {
  uint32_t crc = 0xFFFFFFFFu;
  for (int i = 0; i < len; i++) {
    crc ^= static_cast<uint32_t>(p[i]);
    for (int b = 0; b < 8; b++) {
      const uint32_t mask = -(crc & 1u);
      crc = (crc >> 1) ^ (0xEDB88320u & mask);
    }
  }
  return ~crc;
}
