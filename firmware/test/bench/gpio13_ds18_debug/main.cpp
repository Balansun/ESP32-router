/*
 * Minimal DS18B20 probe on GPIO13 — bench only, not shipped in wroom32 image.
 * pio run -e gpio13_debug -t upload && pio device monitor -e gpio13_debug
 */
#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#ifndef GPIO13_DEBUG_PIN
#define GPIO13_DEBUG_PIN 13
#endif

static OneWire g_one_wire(GPIO13_DEBUG_PIN);
static DallasTemperature g_sensors(&g_one_wire);

static void print_rom_search() {
  g_one_wire.reset_search();
  uint8_t addr[8];
  int found = 0;
  while (g_one_wire.search(addr)) {
    found++;
    Serial.printf("  ROM %d: ", found);
    for (int i = 0; i < 8; i++) {
      Serial.printf("%02X", addr[i]);
    }
    Serial.println();
  }
  if (found == 0) {
    Serial.println("  (no ROM — check data on GPIO, GND, 4.7k to 3.3V)");
  }
}

void setup() {
  Serial.begin(115200);
  delay(800);
  Serial.println();
  Serial.println("=== Balansun GPIO13 DS18B20 debug firmware ===");
  Serial.printf("Data pin: GPIO %d\n", GPIO13_DEBUG_PIN);
  Serial.printf("Chip model: %s\n", ESP.getChipModel());
  Serial.printf("CPU freq: %u MHz\n", getCpuFrequencyMhz());

  const bool reset_ok = g_one_wire.reset();
  Serial.printf("1-Wire reset: %s\n", reset_ok ? "device present (ACK)" : "no ACK");

  g_sensors.begin();
  g_sensors.setWaitForConversion(true);
  const int count = g_sensors.getDeviceCount();
  Serial.printf("Dallas getDeviceCount: %d\n", count);
  Serial.println("ROM search:");
  print_rom_search();
  Serial.println("Polling every 2s...");
}

void loop() {
  const bool reset_ok = g_one_wire.reset();
  Serial.printf("[%lu ms] reset=%s ", millis(), reset_ok ? "ACK" : "none");

  g_sensors.requestTemperatures();
  const int count = g_sensors.getDeviceCount();
  Serial.printf("count=%d ", count);

  if (count <= 0) {
    Serial.println();
  } else {
    for (int i = 0; i < count; i++) {
      const float c = g_sensors.getTempCByIndex(i);
      Serial.printf("T%d=%.2fC ", i, c);
      if (c == DEVICE_DISCONNECTED_C) {
        Serial.print("(DISCONNECTED) ");
      }
    }
    Serial.println();
  }

  delay(2000);
}
