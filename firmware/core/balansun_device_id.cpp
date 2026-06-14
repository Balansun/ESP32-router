/*
 * balansun_device_id.cpp — Arduino device_uid from eFuse MAC.
 */
#include "balansun_device_id.h"

#include "balansun_device_id_logic.h"

#include <esp_mac.h>

static uint64_t balansun_efuse_mac48() {
  uint64_t mac = 0;
  esp_efuse_mac_get_default(reinterpret_cast<uint8_t *>(&mac));
  return mac;
}

const String &balansun_device_uid() {
  static String cached;
  if (cached.length() == 0) {
    char buf[13];
    if (balansun_device_uid_format(balansun_efuse_mac48(), buf, sizeof(buf))) {
      cached = String(buf);
    }
  }
  return cached;
}

bool balansun_apply_default_mqtt_device_name(String &name) {
  if (!balansun_mqtt_device_name_is_factory_default(name.c_str())) return false;
  const String &uid = balansun_device_uid();
  if (uid.length() == 0) return false;
  name = uid;
  return true;
}
