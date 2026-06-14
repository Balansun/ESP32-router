#pragma once

#include <cstdint>
#include <string>

constexpr int kBalansunTempMaxSensors = 2;
constexpr float kBalansunTempInvalidC = -127.0f;

struct BalansunTempSlotConfig {
  bool enabled = false;
  std::string label;
  /** 0 = auto-bind by discovery order on the 1-Wire bus. */
  uint64_t address = 0;
};

struct BalansunTempBusReading {
  float c = kBalansunTempInvalidC;
  bool valid = false;
  uint64_t address = 0;
};

struct BalansunTempSlotState {
  int slot = 0;
  BalansunTempSlotConfig config;
  BalansunTempBusReading reading;
  bool primary = false;
};

bool balansun_temp_logic_is_allowed_gpio(int gpio);
bool balansun_temp_logic_validate_gpio(int gpio, std::string &err);
bool balansun_temp_logic_is_valid_c(float c);
bool balansun_temp_logic_any_slot_enabled(const BalansunTempSlotConfig slots[kBalansunTempMaxSensors]);
void balansun_temp_logic_map_bus_to_slots(const BalansunTempBusReading *bus, int bus_count,
                                       const BalansunTempSlotConfig configs[kBalansunTempMaxSensors],
                                       BalansunTempSlotState out[kBalansunTempMaxSensors]);
int balansun_temp_logic_primary_slot(const BalansunTempSlotState slots[kBalansunTempMaxSensors]);
float balansun_temp_logic_primary_c(const BalansunTempSlotState slots[kBalansunTempMaxSensors]);
std::string balansun_temp_logic_format_address(uint64_t address);
