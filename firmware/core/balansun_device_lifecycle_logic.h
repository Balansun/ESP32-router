#pragma once

#include "balansun_product_caps_logic.h"

#include <cstdint>

enum class BalansunDeviceLifecycle : uint8_t {
  Booting = 0,
  Configured,
  Metering,
  Regulating,
  Suspended,
  Commissioning,
};

struct BalansunDeviceLifecycleInput {
  BalansunProductCaps caps{};
  bool boot_complete = false;
  bool api_telemetry_ready = false;
  bool output_suspend_active = false;
  bool self_test_running = false;
};

BalansunDeviceLifecycle balansun_device_lifecycle_compute(const BalansunDeviceLifecycleInput &in);

const char *balansun_device_lifecycle_wire(BalansunDeviceLifecycle state);
