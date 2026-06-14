#include <gtest/gtest.h>

#include "json_field_parse.h"

TEST(JsonFieldParse, ParseJsonFloatExtractsFloat) {
  const String json = "{\"Pw\":-123.5,\"x\":1}";
  EXPECT_FLOAT_EQ(parse_json_float("\"Pw\"", json), -123.5f);
}

TEST(JsonFieldParse, ParseJsonStringExtractsQuoted) {
  const String json = "{\"source\":\"Linky\",\"n\":1}";
  EXPECT_EQ(parse_json_string("\"source\"", json), "Linky");
}

TEST(JsonFieldParse, ParseJsonLongStopsAtDecimal) {
  const String json = "{\"count\":42.9}";
  EXPECT_EQ(parse_json_long("\"count\"", json), 42L);
}

TEST(JsonFieldParse, PrefilterJsonSlices) {
  const String json = "noise{\"house\":{\"Pw\":10,\"n\":1}}tail";
  const String slice = prefilter_json("{", "\"house\"", json);
  EXPECT_FLOAT_EQ(parse_json_float("\"Pw\"", slice), 10.f);
}
