#include <gtest/gtest.h>

#include "victron_gx_logic.h"

TEST(VictronGxLogic, parse_value_json) {
  float v = 0;
  EXPECT_TRUE(balansun_victron_parse_value_json(R"({"value":850})", v));
  EXPECT_FLOAT_EQ(v, 850.0f);
  EXPECT_FALSE(balansun_victron_parse_value_json(R"({"value":null})", v));
}

TEST(VictronGxLogic, battery_charge_maps_to_export) {
  VictronRawInputs in;
  in.have_battery = true;
  in.battery_w = 850.0f;
  const VictronHousePower hp =
      balansun_victron_map_surplus(in, VictronSurplusMode::BatteryCharge);
  EXPECT_EQ(hp.active_export_w, 850);
  EXPECT_EQ(hp.active_import_w, 0);
}

TEST(VictronGxLogic, battery_discharge_maps_to_import) {
  VictronRawInputs in;
  in.have_battery = true;
  in.battery_w = -420.0f;
  const VictronHousePower hp =
      balansun_victron_map_surplus(in, VictronSurplusMode::BatteryCharge);
  EXPECT_EQ(hp.active_export_w, 0);
  EXPECT_EQ(hp.active_import_w, 420);
}

TEST(VictronGxLogic, grid_export_mode) {
  VictronRawInputs in;
  in.have_grid = true;
  in.grid_w = -300.0f;
  const VictronHousePower hp = balansun_victron_map_surplus(in, VictronSurplusMode::GridExport);
  EXPECT_EQ(hp.active_export_w, 300);
  EXPECT_EQ(hp.active_import_w, 0);
}

TEST(VictronGxLogic, combined_max) {
  VictronRawInputs in;
  in.have_battery = true;
  in.battery_w = 500.0f;
  in.have_grid = true;
  in.grid_w = -800.0f;
  const VictronHousePower hp = balansun_victron_map_surplus(in, VictronSurplusMode::CombinedMax);
  EXPECT_EQ(hp.active_export_w, 800);
}

TEST(VictronGxLogic, surplus_mode_wire_roundtrip) {
  VictronSurplusMode mode = VictronSurplusMode::BatteryCharge;
  EXPECT_TRUE(balansun_victron_surplus_mode_from_wire("combined_max", mode));
  EXPECT_EQ(mode, VictronSurplusMode::CombinedMax);
  EXPECT_STREQ(balansun_victron_surplus_mode_wire(mode), "combined_max");
}
