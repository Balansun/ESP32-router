#include "balansun_temperature_logic.h"

#include <cstdio>
#include <cstring>

bool balansun_temp_logic_is_allowed_gpio(int gpio) {
  return gpio == 13 || gpio == 21 || gpio == 27 || gpio == 33;
}

bool balansun_temp_logic_validate_gpio(int gpio, std::string &err) {
  if (!balansun_temp_logic_is_allowed_gpio(gpio)) {
    err = "temp_gpio must be 13, 21, 27, or 33";
    return false;
  }
  return true;
}

bool balansun_temp_logic_is_valid_c(float c) {
  return c >= -20.0f && c <= 130.0f;
}

bool balansun_temp_logic_any_slot_enabled(const BalansunTempSlotConfig slots[kBalansunTempMaxSensors]) {
  for (int i = 0; i < kBalansunTempMaxSensors; i++) {
    if (slots[i].enabled) return true;
  }
  return false;
}

void balansun_temp_logic_map_bus_to_slots(const BalansunTempBusReading *bus, int bus_count,
                                       const BalansunTempSlotConfig configs[kBalansunTempMaxSensors],
                                       BalansunTempSlotState out[kBalansunTempMaxSensors]) {
  bool bus_used[kBalansunTempMaxSensors] = {false, false};
  if (bus_count > kBalansunTempMaxSensors) bus_count = kBalansunTempMaxSensors;
  for (int s = 0; s < kBalansunTempMaxSensors; s++) {
    out[s].slot = s;
    out[s].config = configs[s];
    out[s].reading = {};
    out[s].primary = false;
    if (!configs[s].enabled) continue;
    int match = -1;
    if (configs[s].address != 0) {
      for (int b = 0; b < bus_count; b++) {
        if (bus[b].address == configs[s].address) {
          match = b;
          break;
        }
      }
    } else {
      for (int b = 0; b < bus_count; b++) {
        if (!bus_used[b]) {
          match = b;
          bus_used[b] = true;
          break;
        }
      }
    }
    if (match >= 0) out[s].reading = bus[match];
  }
  const int primary = balansun_temp_logic_primary_slot(out);
  if (primary >= 0) out[primary].primary = true;
}

int balansun_temp_logic_primary_slot(const BalansunTempSlotState slots[kBalansunTempMaxSensors]) {
  for (int s = 0; s < kBalansunTempMaxSensors; s++) {
    if (slots[s].config.enabled && slots[s].reading.valid) return s;
  }
  return -1;
}

float balansun_temp_logic_primary_c(const BalansunTempSlotState slots[kBalansunTempMaxSensors]) {
  const int p = balansun_temp_logic_primary_slot(slots);
  if (p < 0) return kBalansunTempInvalidC;
  return slots[p].reading.c;
}

std::string balansun_temp_logic_format_address(uint64_t address) {
  if (address == 0) return "";
  char buf[17];
  snprintf(buf, sizeof(buf), "%016llX", static_cast<unsigned long long>(address));
  return std::string(buf);
}
