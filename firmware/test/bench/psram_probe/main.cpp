#include <Arduino.h>
#include <esp_heap_caps.h>

void setup() {
  Serial.begin(115200);
  delay(800);
  Serial.println();
  Serial.println(F("=== PSRAM probe ==="));
  Serial.printf("psramFound: %s\n", psramFound() ? "yes" : "no");
  Serial.printf("ESP.getPsramSize: %u\n", static_cast<unsigned>(ESP.getPsramSize()));
  Serial.printf("ESP.getFreePsram: %u\n", static_cast<unsigned>(ESP.getFreePsram()));
  Serial.printf("heap SPIRAM free: %u\n",
                static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)));
  Serial.printf("heap INTERNAL free: %u\n",
                static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)));
  void *p = heap_caps_malloc(64 * 1024, MALLOC_CAP_SPIRAM);
  Serial.printf("64KB SPIRAM alloc: %s\n", p ? "ok" : "fail");
  if (p) heap_caps_free(p);
  Serial.println(F("done"));
}

void loop() { delay(5000); }
