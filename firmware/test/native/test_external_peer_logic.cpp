#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

#include "external_peer_logic.h"

TEST(ExternalPeerLogic, ParsesMeasurementsJsonFixture) {
  std::ifstream in("firmware/test/fixtures/meters/external/measurements.json");
  std::stringstream ss;
  ss << in.rdbuf();
  ExternalPeerReading rd;
  ASSERT_TRUE(external_peer_logic_parse_measurements_json(ss.str(), rd));
  EXPECT_EQ(rd.house_active_import_w, 150);
  EXPECT_EQ(rd.house_active_export_w, 80);
  EXPECT_EQ(rd.second_active_import_w, 900);
}

TEST(ExternalPeerLogic, RejectsMissingHouse) {
  ExternalPeerReading rd;
  EXPECT_FALSE(external_peer_logic_parse_measurements_json(R"({"second":{"active_import_w":1}})", rd));
}

TEST(ExternalPeerLogic, OptionalExportDefaultsZero) {
  const char *json = R"({"house":{"active_import_w":200}})";
  ExternalPeerReading rd;
  ASSERT_TRUE(external_peer_logic_parse_measurements_json(json, rd));
  EXPECT_EQ(rd.house_active_export_w, 0);
}

TEST(ExternalPeerLogic, ParsesEnergyFieldsFromFixture) {
  std::ifstream in("firmware/test/fixtures/meters/external/measurements.json");
  std::stringstream ss;
  ss << in.rdbuf();
  ExternalPeerReading rd;
  ASSERT_TRUE(external_peer_logic_parse_measurements_json(ss.str(), rd));
  EXPECT_EQ(rd.house_day_energy_import_wh, 1200);
  EXPECT_EQ(rd.house_energy_import_wh, 50000);
  EXPECT_EQ(rd.house_energy_export_wh, 12000);
}

TEST(ExternalPeerLogic, RejectsInvalidImportField) {
  ExternalPeerReading rd;
  EXPECT_FALSE(external_peer_logic_parse_measurements_json(R"({"house":{"active_import_w":}})", rd));
  EXPECT_FALSE(external_peer_logic_parse_measurements_json(R"({"house":{}})", rd));
  EXPECT_FALSE(external_peer_logic_parse_measurements_json(R"({"house":{"active_import_w":)", rd));
}

TEST(ExternalPeerLogic, SecondChannelPartial) {
  ExternalPeerReading rd;
  const char *json = R"({"house":{"active_import_w":1},"second":{"active_export_w":5}})";
  ASSERT_TRUE(external_peer_logic_parse_measurements_json(json, rd));
  EXPECT_EQ(rd.second_active_export_w, 5);
}

TEST(ExternalPeerLogic, ParsesApparentAndDayExportEnergy) {
  ExternalPeerReading rd;
  const char *json =
      R"({"house":{"active_import_w":10,"apparent_import_va":11,"apparent_export_va":0,"energy_day_export_wh":7}})";
  ASSERT_TRUE(external_peer_logic_parse_measurements_json(json, rd));
  EXPECT_EQ(rd.house_apparent_import_va, 11);
  EXPECT_EQ(rd.house_day_energy_export_wh, 7);
}

TEST(ExternalPeerLogic, RejectsNonNumericImport) {
  ExternalPeerReading rd;
  EXPECT_FALSE(external_peer_logic_parse_measurements_json(R"({"house":{"active_import_w":abc}})", rd));
  EXPECT_FALSE(external_peer_logic_parse_measurements_json(R"({"house":{"active_import_w":-}})", rd));
}

TEST(ExternalPeerLogic, ParsesNegativeImport) {
  ExternalPeerReading rd;
  ASSERT_TRUE(external_peer_logic_parse_measurements_json(R"({"house":{"active_import_w":-42}})", rd));
  EXPECT_EQ(rd.house_active_import_w, -42);
}

TEST(ExternalPeerLogic, OptionalEnergyFieldsAbsent) {
  ExternalPeerReading rd;
  ASSERT_TRUE(external_peer_logic_parse_measurements_json(
      R"({"house":{"active_import_w":10,"active_export_w":0}})", rd));
  EXPECT_EQ(rd.house_day_energy_import_wh, 0);
  EXPECT_EQ(rd.house_energy_import_wh, 0);
}

TEST(ExternalPeerLogic, ParsesNegativeExportAndApparent) {
  ExternalPeerReading rd;
  ASSERT_TRUE(external_peer_logic_parse_measurements_json(
      R"({"house":{"active_import_w":0,"active_export_w":-5,"apparent_import_va":2300}})", rd));
  EXPECT_EQ(rd.house_active_export_w, -5);
  EXPECT_EQ(rd.house_apparent_import_va, 2300);
}

TEST(ExternalPeerLogic, RejectsGarbageAfterImportDigits) {
  ExternalPeerReading rd;
  EXPECT_FALSE(external_peer_logic_parse_measurements_json(
      R"({"house":{"active_import_w":abc,"active_export_w":0}})", rd));
}

TEST(ExternalPeerLogic, SecondChannelImportOnly) {
  ExternalPeerReading rd;
  ASSERT_TRUE(external_peer_logic_parse_measurements_json(
      R"({"house":{"active_import_w":1},"second":{"active_import_w":9}})", rd));
  EXPECT_EQ(rd.second_active_import_w, 9);
  EXPECT_EQ(rd.second_active_export_w, 0);
}

TEST(ExternalPeerLogic, ParsesImportWithSpacesAfterColon) {
  ExternalPeerReading rd;
  ASSERT_TRUE(external_peer_logic_parse_measurements_json(
      R"({"house":{"active_import_w":   42,"active_export_w":0}})", rd));
  EXPECT_EQ(rd.house_active_import_w, 42);
}

TEST(ExternalPeerLogic, ParsesImportWithTabsOnlyAfterColon) {
  ExternalPeerReading rd;
  ASSERT_TRUE(external_peer_logic_parse_measurements_json(
      R"({"house":{"active_import_w":		77}})", rd));
  EXPECT_EQ(rd.house_active_import_w, 77);
}

TEST(ExternalPeerLogic, ParsesAllOptionalHouseEnergyFields) {
  ExternalPeerReading rd;
  const char *json =
      R"({"house":{"active_import_w":-12,"active_export_w":3,"apparent_import_va":2300,"apparent_export_va":0,)"
      R"("energy_day_import_wh":100,"energy_day_export_wh":50,"energy_total_import_wh":9000,"energy_total_export_wh":8000}})";
  ASSERT_TRUE(external_peer_logic_parse_measurements_json(json, rd));
  EXPECT_EQ(rd.house_active_import_w, -12);
  EXPECT_EQ(rd.house_day_energy_import_wh, 100);
  EXPECT_EQ(rd.house_energy_export_wh, 8000);
}

TEST(ExternalPeerLogic, ParsesFieldsWithTabWhitespace) {
  ExternalPeerReading rd;
  ASSERT_TRUE(external_peer_logic_parse_measurements_json(
      R"({"house":{"active_import_w":	250,"active_export_w":	10}})", rd));
  EXPECT_EQ(rd.house_active_import_w, 250);
  EXPECT_EQ(rd.house_active_export_w, 10);
}

TEST(ExternalPeerLogic, RejectsMissingNumericHouseFields) {
  ExternalPeerReading rd;
  EXPECT_FALSE(external_peer_logic_parse_measurements_json(R"({"house":{"active_import_w":}})", rd));
  EXPECT_FALSE(external_peer_logic_parse_measurements_json(R"({"house":{}})", rd));
  EXPECT_FALSE(external_peer_logic_parse_measurements_json(R"({"house":{"active_import_w":-}})", rd));
}

TEST(ExternalPeerLogic, RejectsWhitespaceOnlyEnergyFloat) {
  ExternalPeerReading rd;
  const std::string json = "{\"house\":{\"active_import_w\":1,\"energy_day_import_wh\":   ";
  ASSERT_TRUE(external_peer_logic_parse_measurements_json(json, rd));
  EXPECT_EQ(rd.house_day_energy_import_wh, 0);
}

TEST(ExternalPeerLogic, RejectsNonNumericEnergyFloat) {
  ExternalPeerReading rd;
  ASSERT_TRUE(external_peer_logic_parse_measurements_json(
      R"({"house":{"active_import_w":1,"energy_total_export_wh":bad}})", rd));
  EXPECT_EQ(rd.house_energy_export_wh, 0);
}

TEST(ExternalPeerLogic, RejectsImportValueOnlyWhitespace) {
  ExternalPeerReading rd;
  EXPECT_FALSE(external_peer_logic_parse_measurements_json(
      R"({"house":{"active_import_w":   }})", rd));
}
