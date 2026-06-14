#pragma once

#include <Arduino.h>

#if CONFIG_IDF_TARGET_ESP32S3

namespace balansun_bench_serial_detail {

inline HardwareSerial &uart0(void) {
  static HardwareSerial bus(0);
  return bus;
}

}  // namespace balansun_bench_serial_detail

inline void balansun_bench_serial_begin(void) {
  static bool started = false;
  if (started) return;
  balansun_bench_serial_detail::uart0().begin(115200);
  started = true;
}

inline void balansun_bench_log(const __FlashStringHelper *msg) {
  balansun_bench_serial_begin();
  Serial.println(msg);
  balansun_bench_serial_detail::uart0().println(msg);
}

inline void balansun_bench_log(const String &msg) {
  balansun_bench_serial_begin();
  Serial.println(msg);
  balansun_bench_serial_detail::uart0().println(msg);
}

#else

inline void balansun_bench_serial_begin(void) {}
inline void balansun_bench_log(const __FlashStringHelper *msg) { Serial.println(msg); }
inline void balansun_bench_log(const String &msg) { Serial.println(msg); }

#endif
