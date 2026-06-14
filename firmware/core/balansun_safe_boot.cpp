#include "balansun_safe_boot.h"

#include "balansun_pin_map.h"

#include <Arduino.h>

namespace {
constexpr int kBootButtonGpio = 0;
constexpr unsigned long kHoldMs = 5000;
bool g_pin_reset_requested = false;
}  // namespace

void balansun_safe_boot_poll_at_startup() {
  pinMode(kBootButtonGpio, INPUT_PULLUP);
  if (digitalRead(kBootButtonGpio) != LOW) {
    return;
  }
  const unsigned long t0 = millis();
  while (digitalRead(kBootButtonGpio) == LOW) {
    if (millis() - t0 >= kHoldMs) {
      g_pin_reset_requested = true;
      Serial.println(F("SAFE_BOOT: pin map will reset to defaults"));
      return;
    }
    delay(10);
  }
}

bool balansun_safe_boot_pin_reset_requested() { return g_pin_reset_requested; }
