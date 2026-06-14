#include <gtest/gtest.h>

#include <cstdint>
#include <fstream>
#include <sstream>

#include "jsy_mk194_logic.h"

TEST(JsyMk194Logic, ParsesSyntheticModbusFrame) {
  uint8_t bytes[61] = {0};
  bytes[0] = 0x01;
  bytes[1] = 0x03;
  bytes[2] = 0x38;
  bytes[3] = 0x00;
  bytes[4] = 0x23;
  bytes[5] = 0x18;
  bytes[6] = 0xC0;
  bytes[31] = 0x00;
  bytes[32] = 0x00;
  bytes[33] = 0x13;
  bytes[34] = 0x88;
  bytes[59] = 0x00;
  bytes[60] = 0x00;
  JsyMk194Reading rd;
  ASSERT_TRUE(jsy_mk194_parse_modbus_frame(bytes, 61, rd));
  EXPECT_NEAR(rd.voltage_second_v, 230.0f, 0.1f);
  EXPECT_NEAR(rd.frequence_hz, 50.0f, 0.1f);
}

namespace {
bool hex_to_bytes_61(const std::string &hex, uint8_t *out) {
  size_t bi = 0;
  for (size_t i = 0; i + 1 < hex.size() && bi < 61; ++i) {
    if (hex[i] == ' ' || hex[i] == '\n') continue;
    out[bi++] = static_cast<uint8_t>(std::strtoul(hex.substr(i, 2).c_str(), nullptr, 16));
    ++i;
  }
  return bi == 61;
}
}  // namespace

TEST(JsyMk194Logic, ParsesFixtureWithExportSense) {
  std::ifstream in("firmware/test/fixtures/meters/jsy_mk194/sample_frame.hex");
  std::stringstream ss;
  ss << in.rdbuf();
  uint8_t bytes[61] = {0};
  ASSERT_TRUE(hex_to_bytes_61(ss.str(), bytes));
  bytes[27] = 1;
  JsyMk194Reading rd;
  ASSERT_TRUE(jsy_mk194_parse_modbus_frame(bytes, 61, rd));
  EXPECT_GT(rd.second_active_export_w, 0);
  EXPECT_EQ(rd.second_active_import_w, 0);
}

TEST(JsyMk194Logic, ComputesApparentPowerWhenPfPositive) {
  uint8_t bytes[61] = {0};
  bytes[0] = 0x01;
  bytes[1] = 0x03;
  bytes[2] = 0x38;
  bytes[11] = 0x00;
  bytes[12] = 0x98;
  bytes[13] = 0x96;
  bytes[14] = 0x80;
  bytes[19] = 0x00;
  bytes[20] = 0x00;
  bytes[21] = 0x01;
  bytes[22] = 0xF4;
  bytes[27] = 0;
  bytes[28] = 1;
  bytes[43] = 0x00;
  bytes[44] = 0x98;
  bytes[45] = 0x96;
  bytes[46] = 0x80;
  bytes[51] = 0x00;
  bytes[52] = 0x00;
  bytes[53] = 0x01;
  bytes[54] = 0xF4;
  JsyMk194Reading rd;
  ASSERT_TRUE(jsy_mk194_parse_modbus_frame(bytes, 61, rd));
  EXPECT_GT(rd.pf_second, 0.0f);
  EXPECT_EQ(rd.pva_t, 2000);
  EXPECT_GT(rd.house_active_export_w, 0);
  EXPECT_EQ(rd.pva_m, 2000);
}

TEST(JsyMk194Logic, RejectsInvalidFrame) {
  uint8_t bytes[10] = {};
  JsyMk194Reading rd;
  EXPECT_FALSE(jsy_mk194_parse_modbus_frame(nullptr, 61, rd));
  EXPECT_FALSE(jsy_mk194_parse_modbus_frame(bytes, 10, rd));
}
