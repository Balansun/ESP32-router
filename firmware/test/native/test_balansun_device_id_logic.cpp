#include <gtest/gtest.h>

#include "balansun_device_id_logic.h"

TEST(BalansunDeviceIdLogic, FormatFullMac) {
  char buf[13];
  ASSERT_TRUE(balansun_device_uid_format(0xA1B2C3D4E5F6ULL, buf, sizeof(buf)));
  EXPECT_STREQ(buf, "a1b2c3d4e5f6");
}

TEST(BalansunDeviceIdLogic, LeadingZeroPadding) {
  char buf[13];
  ASSERT_TRUE(balansun_device_uid_format(0x000012345678ULL, buf, sizeof(buf)));
  EXPECT_STREQ(buf, "000012345678");
}

TEST(BalansunDeviceIdLogic, MasksUpperBits) {
  char buf[13];
  ASSERT_TRUE(balansun_device_uid_format(0xFFFFA1B2C3D4E5F6ULL, buf, sizeof(buf)));
  EXPECT_STREQ(buf, "a1b2c3d4e5f6");
}

TEST(BalansunDeviceIdLogic, BufferTooSmall) {
  char buf[8];
  EXPECT_FALSE(balansun_device_uid_format(0xA1B2C3D4E5F6ULL, buf, sizeof(buf)));
  EXPECT_FALSE(balansun_device_uid_format(0xA1B2C3D4E5F6ULL, nullptr, 13));
}

TEST(BalansunDeviceIdLogic, FactoryMqttDeviceName) {
  EXPECT_TRUE(balansun_mqtt_device_name_is_factory_default(nullptr));
  EXPECT_TRUE(balansun_mqtt_device_name_is_factory_default(""));
  EXPECT_TRUE(balansun_mqtt_device_name_is_factory_default("balansun"));
  EXPECT_FALSE(balansun_mqtt_device_name_is_factory_default("6809475d1df8"));
  EXPECT_FALSE(balansun_mqtt_device_name_is_factory_default("my_router"));
}
