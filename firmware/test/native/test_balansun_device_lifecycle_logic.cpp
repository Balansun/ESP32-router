#include <gtest/gtest.h>

#include "balansun_device_lifecycle_logic.h"
#include "balansun_product_profile.h"

TEST(BalansunDeviceLifecycle, Booting) {
  BalansunDeviceLifecycleInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  EXPECT_EQ(balansun_device_lifecycle_compute(in), BalansunDeviceLifecycle::Booting);
}

TEST(BalansunDeviceLifecycle, RegulatingWhenReady) {
  BalansunDeviceLifecycleInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  in.boot_complete = true;
  in.api_telemetry_ready = true;
  EXPECT_EQ(balansun_device_lifecycle_compute(in), BalansunDeviceLifecycle::Regulating);
}

TEST(BalansunDeviceLifecycle, MeterOnlyStaysMetering) {
  BalansunDeviceLifecycleInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_METER_ONLY);
  in.boot_complete = true;
  in.api_telemetry_ready = true;
  EXPECT_EQ(balansun_device_lifecycle_compute(in), BalansunDeviceLifecycle::Metering);
}

TEST(BalansunDeviceLifecycle, CommissioningAndSuspended) {
  BalansunDeviceLifecycleInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  in.self_test_running = true;
  EXPECT_EQ(balansun_device_lifecycle_compute(in), BalansunDeviceLifecycle::Commissioning);
  EXPECT_STREQ(balansun_device_lifecycle_wire(BalansunDeviceLifecycle::Commissioning), "commissioning");
  in.self_test_running = false;
  in.boot_complete = true;
  in.api_telemetry_ready = true;
  in.output_suspend_active = true;
  EXPECT_EQ(balansun_device_lifecycle_compute(in), BalansunDeviceLifecycle::Suspended);
  EXPECT_STREQ(balansun_device_lifecycle_wire(BalansunDeviceLifecycle::Suspended), "suspended");
}

TEST(BalansunDeviceLifecycle, ConfiguredWhenBootedNotReady) {
  BalansunDeviceLifecycleInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  in.boot_complete = false;
  in.api_telemetry_ready = false;
  EXPECT_EQ(balansun_device_lifecycle_compute(in), BalansunDeviceLifecycle::Booting);
  in.boot_complete = true;
  in.api_telemetry_ready = false;
  EXPECT_EQ(balansun_device_lifecycle_compute(in), BalansunDeviceLifecycle::Configured);
  in.boot_complete = false;
  EXPECT_EQ(balansun_device_lifecycle_compute(in), BalansunDeviceLifecycle::Booting);
  EXPECT_STREQ(balansun_device_lifecycle_wire(BalansunDeviceLifecycle::Regulating), "regulating");
  EXPECT_STREQ(balansun_device_lifecycle_wire(BalansunDeviceLifecycle::Metering), "metering");
  EXPECT_STREQ(balansun_device_lifecycle_wire(BalansunDeviceLifecycle::Configured), "configured");
  EXPECT_STREQ(balansun_device_lifecycle_wire(static_cast<BalansunDeviceLifecycle>(99)), "booting");
}

TEST(BalansunDeviceLifecycle, TelemetryNotReadyPaths) {
  BalansunDeviceLifecycleInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  in.boot_complete = true;
  in.api_telemetry_ready = false;
  EXPECT_EQ(balansun_device_lifecycle_compute(in), BalansunDeviceLifecycle::Configured);
  in.output_suspend_active = false;
  in.self_test_running = false;
  in.boot_complete = true;
  in.api_telemetry_ready = true;
  EXPECT_EQ(balansun_device_lifecycle_compute(in), BalansunDeviceLifecycle::Regulating);
}
