#include "balansun_device_lifecycle_logic.h"

BalansunDeviceLifecycle balansun_device_lifecycle_compute(const BalansunDeviceLifecycleInput &in) {
  if (in.self_test_running) {
    return BalansunDeviceLifecycle::Commissioning;
  }
  if (!in.boot_complete) {
    return BalansunDeviceLifecycle::Booting;
  }
  if (!in.api_telemetry_ready) {
    return BalansunDeviceLifecycle::Configured;
  }
  if (in.output_suspend_active) {
    return BalansunDeviceLifecycle::Suspended;
  }
  if (balansun_product_caps_has(in.caps, BalansunCap::SurplusRegulation)) {
    return BalansunDeviceLifecycle::Regulating;
  }
  return BalansunDeviceLifecycle::Metering;
}

const char *balansun_device_lifecycle_wire(BalansunDeviceLifecycle state) {
  switch (state) {
    case BalansunDeviceLifecycle::Configured:
      return "configured";
    case BalansunDeviceLifecycle::Metering:
      return "metering";
    case BalansunDeviceLifecycle::Regulating:
      return "regulating";
    case BalansunDeviceLifecycle::Suspended:
      return "suspended";
    case BalansunDeviceLifecycle::Commissioning:
      return "commissioning";
    case BalansunDeviceLifecycle::Booting:
    default:
      return "booting";
  }
}
