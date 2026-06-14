#include <gtest/gtest.h>

#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>

#include "jsy_mk333_logic.h"

namespace {
bool hex_to_bytes(const std::string &hex, uint8_t *out, int max_len, int &len_out) {
  len_out = 0;
  for (size_t i = 0; i + 1 < hex.size() && len_out < max_len; i += 2) {
    if (hex[i] == '\n' || hex[i] == ' ') continue;
    const auto byte = static_cast<uint8_t>(std::strtoul(hex.substr(i, 2).c_str(), nullptr, 16));
    out[len_out++] = byte;
  }
  return len_out > 0;
}
}  // namespace

TEST(JsyMk333Logic, ParsesSampleFrame) {
  std::ifstream in("firmware/test/fixtures/meters/jsy_mk333/sample_frame.hex");
  std::stringstream ss;
  ss << in.rdbuf();
  uint8_t bytes[200];
  int n = 0;
  ASSERT_TRUE(hex_to_bytes(ss.str(), bytes, 200, n));
  ASSERT_EQ(n, 141);
  JsyMk333Reading rd;
  ASSERT_TRUE(jsy_mk333_parse_modbus_frame(bytes, n, rd));
  EXPECT_EQ(rd.house_active_import_w, 2000);
  EXPECT_FALSE(rd.injection);
}

TEST(JsyMk333Logic, RejectsBadLength) {
  uint8_t bytes[10] = {};
  JsyMk333Reading rd;
  EXPECT_FALSE(jsy_mk333_parse_modbus_frame(nullptr, 141, rd));
  EXPECT_FALSE(jsy_mk333_parse_modbus_frame(bytes, 10, rd));
}

TEST(JsyMk333Logic, InjectionPath) {
  std::ifstream in("firmware/test/fixtures/meters/jsy_mk333/sample_frame.hex");
  std::stringstream ss;
  ss << in.rdbuf();
  uint8_t bytes[200];
  int n = 0;
  ASSERT_TRUE(hex_to_bytes(ss.str(), bytes, 200, n));
  bytes[104] = 0x08;
  JsyMk333Reading rd;
  ASSERT_TRUE(jsy_mk333_parse_modbus_frame(bytes, n, rd));
  EXPECT_TRUE(rd.injection);
  EXPECT_EQ(rd.house_active_import_w, 0);
  EXPECT_GT(rd.house_active_export_w, 0);
}

TEST(JsyMk333Logic, NegativePhaseSigns) {
  std::ifstream in("firmware/test/fixtures/meters/jsy_mk333/sample_frame.hex");
  std::stringstream ss;
  ss << in.rdbuf();
  uint8_t bytes[200];
  int n = 0;
  ASSERT_TRUE(hex_to_bytes(ss.str(), bytes, 200, n));
  bytes[104] = 0x07;
  JsyMk333Reading rd;
  ASSERT_TRUE(jsy_mk333_parse_modbus_frame(bytes, n, rd));
  EXPECT_FALSE(rd.injection);
}
