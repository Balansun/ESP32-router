#include "balansun_psram.h"

#include <Arduino.h>
#include <esp_heap_caps.h>

/*
 * balansun_psram.cpp — Arduino glue for the opportunistic PSRAM cache.
 * Pure policy lives in balansun_psram_logic.h (unit-tested on host).
 */

bool balansun_psram_available() { return psramFound(); }

size_t balansun_psram_size() { return ESP.getPsramSize(); }

size_t balansun_psram_free() { return ESP.getFreePsram(); }

void balansun_psram_log_boot() {
  if (balansun_psram_available()) {
    Serial.printf("PSRAM: %u bytes total, %u free — caching in PSRAM\n",
                  static_cast<unsigned>(balansun_psram_size()),
                  static_cast<unsigned>(balansun_psram_free()));
  } else {
    Serial.println("PSRAM: not present — caches use internal heap");
  }
}

void *balansun_cache_malloc(size_t bytes) {
  if (bytes == 0) bytes = 1;
  if (psram_should_use(bytes, balansun_psram_available())) {
    void *p = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM);
    if (p) return p;  // fall through to heap on PSRAM exhaustion
  }
  return malloc(bytes);
}

void balansun_cache_free(void *p) { free(p); }
