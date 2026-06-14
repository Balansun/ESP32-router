#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

#include "pmqtt_logic.h"

namespace {
std::string read_file(const char *path) {
  std::ifstream in(path);
  std::stringstream ss;
  ss << in.rdbuf();
  return ss.str();
}
}  // namespace

TEST(PmqttLogic, ClassifiesHouseSnapshot) {
  EXPECT_EQ(pmqtt_logic_classify_payload(read_file("firmware/test/fixtures/meters/pmqtt/house_snapshot.json")),
            PmqttPayloadKind::HouseSnapshot);
}

TEST(PmqttLogic, ParsesPwPf) {
  PmqttPwReading rd;
  ASSERT_TRUE(pmqtt_logic_parse_pw_schema(read_file("firmware/test/fixtures/meters/pmqtt/pw_pf.json"), "Pw,Pf", rd));
  EXPECT_EQ(rd.house_active_import_w, 0);
  EXPECT_EQ(rd.house_active_export_w, 1500);
}

TEST(PmqttLogic, ClassifiesPwPfAndNone) {
  EXPECT_EQ(pmqtt_logic_classify_payload(R"({"power_w":100})"), PmqttPayloadKind::PwPf);
  EXPECT_EQ(pmqtt_logic_classify_payload(R"({"active_power_w":-50})"), PmqttPayloadKind::PwPf);
  EXPECT_EQ(pmqtt_logic_classify_payload(R"({"other":1})"), PmqttPayloadKind::None);
}

TEST(PmqttLogic, ParsesPositiveImportWithPf) {
  PmqttPwReading rd;
  ASSERT_TRUE(pmqtt_logic_parse_pw_schema(R"({"Pw":1200,"Pf":0.5,"x":0})", "Pw,Pf", rd));
  EXPECT_EQ(rd.house_active_import_w, 1200);
  EXPECT_EQ(rd.house_active_export_w, 0);
  EXPECT_GT(rd.house_apparent_import_va, rd.house_active_import_w);
}

TEST(PmqttLogic, ParsesPowerWSchema) {
  PmqttPwReading rd;
  ASSERT_TRUE(pmqtt_logic_parse_pw_schema(R"({"power_w":-800,"x":0})", "power_w", rd));
  EXPECT_EQ(rd.house_active_export_w, 800);
}

TEST(PmqttLogic, ParsesActivePowerWSchema) {
  PmqttPwReading rd;
  ASSERT_TRUE(pmqtt_logic_parse_pw_schema(R"({"active_power_w":-500,"x":0})", "active_power_w", rd));
  EXPECT_EQ(rd.house_active_export_w, 500);
}

TEST(PmqttLogic, ParsesActivePowerWWithoutPwKey) {
  PmqttPwReading rd;
  ASSERT_TRUE(pmqtt_logic_parse_pw_schema(R"({"active_power_w":900,"x":0})", "active_power_w", rd));
  EXPECT_EQ(rd.house_active_import_w, 900);
}

TEST(PmqttLogic, RejectsUnknownSchema) {
  PmqttPwReading rd;
  EXPECT_FALSE(pmqtt_logic_parse_pw_schema(R"({"Pw":100})", "other", rd));
  EXPECT_FALSE(pmqtt_logic_parse_pw_schema(R"({"Pw":100})", "xPw", rd));
}

TEST(PmqttLogic, ClampsPfAndHandlesZeroPfImport) {
  PmqttPwReading rd;
  ASSERT_TRUE(pmqtt_logic_parse_pw_schema(R"({"Pw":1000,"Pf":2.5,"x":0})", "Pw,Pf", rd));
  EXPECT_EQ(rd.house_active_import_w, 1000);
  EXPECT_EQ(rd.pf, 1.0f);
  ASSERT_TRUE(pmqtt_logic_parse_pw_schema(R"({"Pw":500,"Pf":0,"x":0})", "Pw,Pf", rd));
  EXPECT_EQ(rd.house_apparent_import_va, 500);
}

TEST(PmqttLogic, ExportWithPf) {
  PmqttPwReading rd;
  ASSERT_TRUE(pmqtt_logic_parse_pw_schema(R"({"Pw":-600,"Pf":0.5,"x":0})", "Pw,Pf", rd));
  EXPECT_EQ(rd.house_active_export_w, 600);
  EXPECT_GT(rd.house_apparent_export_va, rd.house_active_export_w);
}

TEST(PmqttLogic, PowerWZeroDoesNotMatch) {
  PmqttPwReading rd;
  EXPECT_FALSE(pmqtt_logic_parse_pw_schema(R"({"power_w":0})", "power_w", rd));
}

TEST(PmqttLogic, ExportUsesRawPowerWhenPfNearZero) {
  PmqttPwReading rd;
  ASSERT_TRUE(pmqtt_logic_parse_pw_schema(R"({"Pw":-400,"Pf":0})", "Pw,Pf", rd));
  EXPECT_EQ(rd.house_active_export_w, 400);
  EXPECT_EQ(rd.house_apparent_export_va, 400);
}

TEST(PmqttLogic, ClassifiesPwSubstring) {
  EXPECT_EQ(pmqtt_logic_classify_payload(R"({"x":"Pw"})"), PmqttPayloadKind::PwPf);
}

TEST(PmqttLogic, SchemaTokenCommaBoundaries) {
  PmqttPwReading rd;
  ASSERT_TRUE(pmqtt_logic_parse_pw_schema(R"({"Pw":100,"x":0})", "foo,Pw,bar", rd));
  EXPECT_EQ(rd.house_active_import_w, 100);
  EXPECT_FALSE(pmqtt_logic_parse_pw_schema(R"({"Pw":100,"x":0})", "xPw", rd));
  EXPECT_FALSE(pmqtt_logic_parse_pw_schema(R"({"Pw":100,"x":0})", "PwExtra", rd));
}

TEST(PmqttLogic, ActivePowerZeroFallsThroughToPowerW) {
  PmqttPwReading rd;
  ASSERT_TRUE(pmqtt_logic_parse_pw_schema(
      R"({"active_power_w":0,"power_w":-300,"x":0})", "active_power_w,power_w", rd));
  EXPECT_EQ(rd.house_active_export_w, 300);
}

TEST(PmqttLogic, PwZeroStillParses) {
  PmqttPwReading rd;
  ASSERT_TRUE(pmqtt_logic_parse_pw_schema(R"({"Pw":0})", "Pw", rd));
  EXPECT_EQ(rd.house_active_import_w, 0);
  EXPECT_EQ(rd.house_active_export_w, 0);
}
