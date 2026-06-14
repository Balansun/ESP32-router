#pragma once

#include <cstdint>

constexpr float kBalansunTemperatureInvalidC = -127.0f;

struct TemperatureReading {
  float c = kBalansunTemperatureInvalidC;
  bool valid = false;
  uint64_t address = 0;
};
