#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

#include "shelly_em_logic.h"

TEST(ShellyEmLogic, ParsesMonophaseFixture) {
  std::ifstream in("firmware/test/fixtures/meters/shelly_em/monophase.json");
  std::ostringstream ss;
  ss << in.rdbuf();
  ShellyEmMonoReading rd;
  ASSERT_TRUE(shelly_em_logic_parse_monophase_json(ss.str().c_str(), rd));
  EXPECT_LT(rd.power_w, 0);
  EXPECT_EQ(rd.active_export_w, 1500);
  EXPECT_NEAR(rd.voltage_v, 230.5f, 0.1f);
}

TEST(ShellyEmLogic, RejectsNullAndMissingTrue) {
  ShellyEmMonoReading rd;
  EXPECT_FALSE(shelly_em_logic_parse_monophase_json(nullptr, rd));
  EXPECT_FALSE(shelly_em_logic_parse_monophase_json(R"({"power":100})", rd));
}

TEST(ShellyEmLogic, ParsesPositiveImport) {
  ShellyEmMonoReading rd;
  ASSERT_TRUE(shelly_em_logic_parse_monophase_json(R"({"power":1200,"voltage":230,"pf":0.9,"total":100,"total_returned":0,true})", rd));
  EXPECT_EQ(rd.active_import_w, 1200);
  EXPECT_EQ(rd.active_export_w, 0);
  EXPECT_GT(rd.apparent_import_va, 0);
}

TEST(ShellyEmLogic, ZeroPfSkipsApparent) {
  ShellyEmMonoReading rd;
  ASSERT_TRUE(shelly_em_logic_parse_monophase_json(R"({"power":800,"voltage":230,"pf":0,"total":1,"total_returned":2,true})", rd));
  EXPECT_EQ(rd.active_import_w, 800);
  EXPECT_EQ(rd.apparent_import_va, 0);
  ASSERT_TRUE(shelly_em_logic_parse_monophase_json(R"({"power":-600,"voltage":230,"pf":0,"total":1,"total_returned":2,true})", rd));
  EXPECT_EQ(rd.active_export_w, 600);
  EXPECT_EQ(rd.apparent_export_va, 0);
}
