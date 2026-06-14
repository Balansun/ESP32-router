#pragma once

#include <cstddef>

/** Sidecar NVS storage for PmqttBindingsJson (too large for EEPROM extension tail). */
bool balansun_pmqtt_bindings_nvs_save(const char *json, size_t len);
bool balansun_pmqtt_bindings_nvs_load(char *out, size_t outCap, size_t *outLen);
void balansun_pmqtt_bindings_nvs_clear(void);
