#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

#include "json_flat_meter_logic.h"

namespace {
std::string read_file(const char *path) {
  std::ifstream in(path);
  std::stringstream ss;
  ss << in.rdbuf();
  return ss.str();
}
}  // namespace

TEST(JsonFlatMeterLogic, ParsesSmartGFixture) {
  JsonFlatMeterReading rd;
  ASSERT_TRUE(json_flat_meter_logic_parse_smartg(read_file("firmware/test/fixtures/meters/smartg/api_read.json"), rd));
  EXPECT_EQ(rd.active_import_w, 1250);
  EXPECT_EQ(rd.active_export_w, 320);
}

TEST(JsonFlatMeterLogic, ParsesHomeWizardFixture) {
  JsonFlatMeterReading rd;
  ASSERT_TRUE(json_flat_meter_logic_parse_homewizard(read_file("firmware/test/fixtures/meters/homewizard/api_v1_data.json"), rd));
  EXPECT_EQ(rd.active_import_w, 0);
  EXPECT_EQ(rd.active_export_w, 1850);
}

TEST(JsonFlatMeterLogic, ParsesEnphaseNetFixture) {
  JsonFlatMeterReading rd;
  ASSERT_TRUE(json_flat_meter_logic_parse_enphase_net(read_file("firmware/test/fixtures/meters/enphase/net_consumption.json"), rd));
  EXPECT_EQ(rd.active_import_w, 0);
  EXPECT_EQ(rd.active_export_w, 2400);
}

TEST(JsonFlatMeterLogic, HomeWizardImportPositive) {
  JsonFlatMeterReading rd;
  ASSERT_TRUE(json_flat_meter_logic_parse_homewizard(
      R"({"active_power_w":900,"total_power_import_t1_kwh":0,"total_power_import_t2_kwh":0})", rd));
  EXPECT_EQ(rd.active_import_w, 900);
  EXPECT_EQ(rd.active_export_w, 0);
}

TEST(JsonFlatMeterLogic, EnphaseImportAndApparent) {
  JsonFlatMeterReading rd;
  ASSERT_TRUE(json_flat_meter_logic_parse_enphase_net(R"({"actPower":500,"apprntPwr":600,"x":0})", rd));
  EXPECT_EQ(rd.active_import_w, 500);
  EXPECT_EQ(rd.apparent_import_va, 600);
}

TEST(JsonFlatMeterLogic, SmartGRejectsZeroPower) {
  JsonFlatMeterReading rd;
  EXPECT_FALSE(json_flat_meter_logic_parse_smartg(R"({"PowerDelivered_total":0,"PowerReturned_total":0})", rd));
}

TEST(JsonFlatMeterLogic, EnphaseNegativeApparent) {
  JsonFlatMeterReading rd;
  ASSERT_TRUE(json_flat_meter_logic_parse_enphase_net(R"({"actPower":100,"apprntPwr":-400,"x":0})", rd));
  EXPECT_EQ(rd.apparent_export_va, 400);
}

TEST(JsonFlatMeterLogic, HomeWizardExportPath) {
  JsonFlatMeterReading rd;
  ASSERT_TRUE(json_flat_meter_logic_parse_homewizard(
      R"({"active_power_w":-750,"total_power_import_t1_kwh":0,"total_power_import_t2_kwh":0})", rd));
  EXPECT_EQ(rd.active_import_w, 0);
  EXPECT_EQ(rd.active_export_w, 750);
}

TEST(JsonFlatMeterLogic, HomeWizardEnergyOnlyReturn) {
  JsonFlatMeterReading rd;
  ASSERT_TRUE(json_flat_meter_logic_parse_homewizard(
      R"({"active_power_w":0,"total_power_import_t1_kwh":2.5,"total_power_import_t2_kwh":0})", rd));
  EXPECT_EQ(rd.active_import_w, 0);
  EXPECT_EQ(rd.active_export_w, 0);
  EXPECT_EQ(rd.energy_import_wh, 2500);
}

TEST(JsonFlatMeterLogic, EnphaseZeroReject) {
  JsonFlatMeterReading rd;
  EXPECT_FALSE(json_flat_meter_logic_parse_enphase_net(R"({"actPower":0,"apprntPwr":0})", rd));
}

TEST(JsonFlatMeterLogic, SmartGExportOnly) {
  JsonFlatMeterReading rd;
  ASSERT_TRUE(json_flat_meter_logic_parse_smartg(
      R"({"PowerDelivered_total":0,"PowerReturned_total":420,"EnergyReturnedTariff1":0})", rd));
  EXPECT_EQ(rd.active_import_w, 0);
  EXPECT_EQ(rd.active_export_w, 420);
}

TEST(JsonFlatMeterLogic, EnphaseExportActPower) {
  JsonFlatMeterReading rd;
  ASSERT_TRUE(json_flat_meter_logic_parse_enphase_net(R"({"actPower":-300,"apprntPwr":0})", rd));
  EXPECT_EQ(rd.active_import_w, 0);
  EXPECT_EQ(rd.active_export_w, 300);
}

TEST(JsonFlatMeterLogic, HomeWizardZeroPowerNoEnergy) {
  JsonFlatMeterReading rd;
  EXPECT_FALSE(json_flat_meter_logic_parse_homewizard(
      R"({"active_power_w":0,"total_power_import_t1_kwh":0,"total_power_import_t2_kwh":0})", rd));
}

TEST(JsonFlatMeterLogic, EnphaseApparentOnly) {
  JsonFlatMeterReading rd;
  ASSERT_TRUE(json_flat_meter_logic_parse_enphase_net(R"({"actPower":0,"apprntPwr":150,"x":0})", rd));
  EXPECT_EQ(rd.apparent_import_va, 150);
}
