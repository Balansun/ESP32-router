#include <gtest/gtest.h>

#include "balansun_temperature_logic.h"

TEST(BalansunTemperatureLogic, GpioAllowList) {
  EXPECT_TRUE(balansun_temp_logic_is_allowed_gpio(13));
  EXPECT_TRUE(balansun_temp_logic_is_allowed_gpio(21));
  EXPECT_FALSE(balansun_temp_logic_is_allowed_gpio(14));
  std::string err;
  EXPECT_FALSE(balansun_temp_logic_validate_gpio(99, err));
  EXPECT_NE(err.find("temp_gpio"), std::string::npos);
}

TEST(BalansunTemperatureLogic, PrimarySlotPrefersSlot0) {
  BalansunTempSlotConfig cfg[2] = {};
  cfg[0].enabled = true;
  cfg[1].enabled = true;
  BalansunTempBusReading bus[2] = {};
  bus[0].c = 40.0f;
  bus[0].valid = true;
  bus[0].address = 0x28AA;
  bus[1].c = 20.0f;
  bus[1].valid = true;
  bus[1].address = 0x28BB;
  BalansunTempSlotState out[2] = {};
  balansun_temp_logic_map_bus_to_slots(bus, 2, cfg, out);
  EXPECT_EQ(balansun_temp_logic_primary_slot(out), 0);
  EXPECT_FLOAT_EQ(balansun_temp_logic_primary_c(out), 40.0f);
  EXPECT_TRUE(out[0].primary);
}

TEST(BalansunTemperatureLogic, DisabledSlotSkipped) {
  BalansunTempSlotConfig cfg[2] = {};
  cfg[0].enabled = false;
  cfg[1].enabled = true;
  cfg[1].address = 0x28BB;
  BalansunTempBusReading bus[2] = {};
  bus[0].c = 40.0f;
  bus[0].valid = true;
  bus[0].address = 0x28AA;
  bus[1].c = 22.5f;
  bus[1].valid = true;
  bus[1].address = 0x28BB;
  BalansunTempSlotState out[2] = {};
  balansun_temp_logic_map_bus_to_slots(bus, 2, cfg, out);
  EXPECT_EQ(balansun_temp_logic_primary_slot(out), 1);
  EXPECT_FLOAT_EQ(balansun_temp_logic_primary_c(out), 22.5f);
}

TEST(BalansunTemperatureLogic, ValidCAndBusHelpers) {
  EXPECT_TRUE(balansun_temp_logic_is_valid_c(21.5f));
  EXPECT_FALSE(balansun_temp_logic_is_valid_c(-127.0f));
  EXPECT_FALSE(balansun_temp_logic_is_valid_c(200.0f));
  BalansunTempSlotConfig cfg[2] = {};
  EXPECT_FALSE(balansun_temp_logic_any_slot_enabled(cfg));
  cfg[1].enabled = true;
  EXPECT_TRUE(balansun_temp_logic_any_slot_enabled(cfg));
  EXPECT_EQ(balansun_temp_logic_format_address(0), "");
  EXPECT_EQ(balansun_temp_logic_format_address(0x28AABBCCDDEEFF00ULL), "28AABBCCDDEEFF00");
  EXPECT_TRUE(balansun_temp_logic_is_allowed_gpio(27));
  EXPECT_TRUE(balansun_temp_logic_is_allowed_gpio(33));
}

TEST(BalansunTemperatureLogic, NoPrimaryWhenInvalid) {
  BalansunTempSlotConfig cfg[2] = {};
  cfg[0].enabled = true;
  BalansunTempBusReading bus[1] = {};
  bus[0].c = 40.0f;
  bus[0].valid = false;
  bus[0].address = 0x28AA;
  BalansunTempSlotState out[2] = {};
  balansun_temp_logic_map_bus_to_slots(bus, 1, cfg, out);
  EXPECT_EQ(balansun_temp_logic_primary_slot(out), -1);
  EXPECT_FLOAT_EQ(balansun_temp_logic_primary_c(out), kBalansunTempInvalidC);
}

TEST(BalansunTemperatureLogic, AddressBinding) {
  BalansunTempSlotConfig cfg[2] = {};
  cfg[0].enabled = true;
  cfg[0].address = 0x28BB;
  cfg[1].enabled = true;
  cfg[1].address = 0x28AA;
  BalansunTempBusReading bus[2] = {};
  bus[0].c = 10.0f;
  bus[0].valid = true;
  bus[0].address = 0x28AA;
  bus[1].c = 50.0f;
  bus[1].valid = true;
  bus[1].address = 0x28BB;
  BalansunTempSlotState out[2] = {};
  balansun_temp_logic_map_bus_to_slots(bus, 2, cfg, out);
  EXPECT_FLOAT_EQ(out[0].reading.c, 50.0f);
  EXPECT_FLOAT_EQ(out[1].reading.c, 10.0f);
}

TEST(BalansunTemperatureLogic, AutoBindAndMissingAddress) {
  BalansunTempSlotConfig cfg[2] = {};
  cfg[0].enabled = true;
  cfg[1].enabled = true;
  BalansunTempBusReading bus[3] = {};
  bus[0].c = 11.0f;
  bus[0].valid = true;
  bus[0].address = 0x01;
  bus[1].c = 22.0f;
  bus[1].valid = true;
  bus[1].address = 0x02;
  bus[2].c = 33.0f;
  bus[2].valid = true;
  bus[2].address = 0x03;
  BalansunTempSlotState out[2] = {};
  balansun_temp_logic_map_bus_to_slots(bus, 3, cfg, out);
  EXPECT_FLOAT_EQ(out[0].reading.c, 11.0f);
  EXPECT_FLOAT_EQ(out[1].reading.c, 22.0f);

  BalansunTempSlotConfig cfg2[2] = {};
  cfg2[0].enabled = true;
  cfg2[0].address = 0xDEADBEEF;
  BalansunTempBusReading bus2[1] = {};
  bus2[0].c = 40.0f;
  bus2[0].valid = true;
  bus2[0].address = 0x28AA;
  BalansunTempSlotState out2[2] = {};
  balansun_temp_logic_map_bus_to_slots(bus2, 1, cfg2, out2);
  EXPECT_FALSE(out2[0].reading.valid);
  EXPECT_EQ(balansun_temp_logic_primary_slot(out2), -1);
}
