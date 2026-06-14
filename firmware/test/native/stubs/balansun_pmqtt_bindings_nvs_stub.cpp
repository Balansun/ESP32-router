#include "balansun_pmqtt_bindings_nvs.h"

#include <cstring>

bool balansun_pmqtt_bindings_nvs_save(const char *json, size_t len) {
  (void)json;
  (void)len;
  return true;
}

bool balansun_pmqtt_bindings_nvs_load(char *out, size_t outCap, size_t *outLen) {
  (void)out;
  (void)outCap;
  if (outLen) *outLen = 0;
  return false;
}

void balansun_pmqtt_bindings_nvs_clear(void) {}
