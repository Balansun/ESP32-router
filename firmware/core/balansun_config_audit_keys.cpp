#include "balansun_config_audit_keys.h"

#include <cstring>

bool balansun_config_audit_is_secret_key(const char *key) {
  if (!key) return true;
  static const char *kSecrets[] = {
      "mqtt_password", "http_api_password", "enphase_password", "fleet_trust_key",
      "arduino_ota_password", "wifi_pass", "password", "mqtt_user", nullptr};
  for (int i = 0; kSecrets[i]; i++) {
    if (strcmp(key, kSecrets[i]) == 0) return true;
  }
  if (strstr(key, "password") != nullptr) return true;
  if (strstr(key, "secret") != nullptr) return true;
  return false;
}
