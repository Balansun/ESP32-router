#pragma once

#include <cstdint>

enum class BalansunCap : uint32_t {
  SurplusRegulation = 1u << 0,
  TriacDimming = 1u << 1,
  MultiAction = 1u << 2,
  MqttHaDiscovery = 1u << 3,
  SourceTestInject = 1u << 4,
  VacationMode = 1u << 5,
  SelfTestTriac = 1u << 6,
  PwmChannel = 1u << 7,
  StatusLedRgb = 1u << 8,
};

struct BalansunProductCaps {
  uint32_t mask = 0;
  const char *profile_wire = "full_router";
};

BalansunProductCaps balansun_product_caps_for_role(int role);
BalansunProductCaps balansun_product_caps_for_profile(int product_profile);
bool balansun_product_caps_has(const BalansunProductCaps &caps, BalansunCap cap);
const char *balansun_product_profile_wire(int product_profile);
const char *balansun_product_cap_wire(BalansunCap cap);
