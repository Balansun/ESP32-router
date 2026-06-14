#include <gtest/gtest.h>

#include "balansun_meter_http_trim_logic.h"

TEST(MeterHttpTrim, extractsInnerObject) {
  std::string inner;
  EXPECT_TRUE(balansun_meter_http_trim_json_object(R"({"a":1,"b":2})", inner));
  EXPECT_EQ(inner, R"("a":1,"b":2)");
}

TEST(MeterHttpTrim, rejectsMissingBrace) {
  std::string inner;
  EXPECT_FALSE(balansun_meter_http_trim_json_object("not json", inner));
}
