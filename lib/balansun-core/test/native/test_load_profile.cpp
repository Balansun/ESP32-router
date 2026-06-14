#include <gtest/gtest.h>

#include <balansun/load_profile.h>

TEST(LoadProfile, WireRoundTrip) {
  EXPECT_STREQ(balansun_load_profile_wire(LoadProfile::WaterHeaterTriac), "water_heater_triac");
  EXPECT_STREQ(balansun_load_profile_wire(LoadProfile::GenericTriac), "generic_triac");
  EXPECT_EQ(balansun_load_profile_from_wire("generic_triac"), LoadProfile::GenericTriac);
}

TEST(LoadProfile, MigratesLegacyRadiatorWires) {
  EXPECT_EQ(balansun_load_profile_from_wire("radiator_pilot_wire_remote"), LoadProfile::WaterHeaterTriac);
  EXPECT_EQ(balansun_load_profile_from_wire("radiator_triac_remote"), LoadProfile::WaterHeaterTriac);
}

TEST(LoadProfile, CapabilityFlags) {
  EXPECT_TRUE(balansun_load_profile_supports_triac(LoadProfile::WaterHeaterTriac));
  EXPECT_TRUE(balansun_load_profile_supports_triac(LoadProfile::GenericTriac));
}
