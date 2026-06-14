/*
 * balansun_device_id_logic.cpp — device_uid string from eFuse MAC (48-bit).
 */
#include "balansun_device_id_logic.h"

#include <cstring>

bool balansun_device_uid_format(uint64_t efuse_mac48, char *out, size_t out_len) {
  if (!out || out_len < 13) return false;
  uint64_t mac = efuse_mac48 & 0xFFFFFFFFFFFFULL;
  static const char kHex[] = "0123456789abcdef";
  for (int i = 11; i >= 0; --i) {
    out[i] = kHex[mac & 0xf];
    mac >>= 4;
  }
  out[12] = '\0';
  return true;
}

bool balansun_mqtt_device_name_is_factory_default(const char *name) {
  if (!name || name[0] == '\0') return true;
  return strcmp(name, "balansun") == 0;
}
