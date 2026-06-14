#include <gtest/gtest.h>

#include "api_access_token.h"
#include "api_session_logic.h"
#include "api_token_logic.h"

TEST(ApiTokenLogic, Sha256HexDeterministicStub) {
  const std::string a = api_token_sha256_hex("abc");
  const std::string b = api_token_sha256_hex("abc");
  EXPECT_EQ(a, b);
  EXPECT_EQ(a.length(), 64u);
}

TEST(ApiTokenLogic, LabelValidation) {
  std::string err;
  EXPECT_TRUE(api_token_validate_label("", err));
  EXPECT_TRUE(api_token_validate_label("Home Assistant", err));
  EXPECT_FALSE(api_token_validate_label(std::string(25, 'a'), err));
  EXPECT_FALSE(api_token_validate_label("bad\n", err));
}

TEST(ApiTokenLogic, LowestFreeId) {
  EXPECT_EQ(api_token_lowest_free_id({}), 1);
  const std::vector<uint8_t> used = {1, 3};
  EXPECT_EQ(api_token_lowest_free_id(used), 2);
  const std::vector<uint8_t> used2 = {1, 2, 3, 4};
  EXPECT_EQ(api_token_lowest_free_id(used2), 5);
}

TEST(ApiTokenLogic, Sha256EmptyToken) {
  EXPECT_TRUE(api_token_sha256_hex("").empty());
}

TEST(ApiTokenLogic, HexConstantTimeEq) {
  EXPECT_TRUE(api_token_hex_constant_time_eq("aa", "aa"));
  EXPECT_FALSE(api_token_hex_constant_time_eq("aa", "ab"));
  EXPECT_FALSE(api_token_hex_constant_time_eq("a", "aa"));
}

TEST(ApiTokenLogic, LabelAsciiBoundaries) {
  std::string err;
  EXPECT_TRUE(api_token_validate_label(std::string(1, static_cast<char>(32)), err));
  EXPECT_TRUE(api_token_validate_label(std::string(1, static_cast<char>(126)), err));
  EXPECT_FALSE(api_token_validate_label(std::string(1, static_cast<char>(31)), err));
}

TEST(ApiTokenLogic, LowestFreeIdExhausted) {
  std::vector<uint8_t> all;
  all.reserve(255);
  for (int i = 1; i <= 255; i++) all.push_back(static_cast<uint8_t>(i));
  EXPECT_EQ(api_token_lowest_free_id(all), 0);
}

TEST(ApiTokenLogic, LowestFreeIdSkipsTakenSlot) {
  const std::vector<uint8_t> used = {3, 1};
  EXPECT_EQ(api_token_lowest_free_id(used), 2);
}

TEST(ApiAccessToken, CreateVerifyRevoke) {
  api_access_tokens_clear();
  std::string token;
  uint8_t id = 0;
  std::string labelOut;
  std::string err;
  ASSERT_TRUE(api_access_tokens_create("HA", token, id, labelOut, err));
  EXPECT_EQ(labelOut, "HA");
  EXPECT_TRUE(api_logic_is_valid_session_token_format(token));
  EXPECT_TRUE(api_access_tokens_verify_bearer(token));
  EXPECT_FALSE(api_access_tokens_verify_bearer("deadbeef"));
  ASSERT_TRUE(api_access_tokens_revoke(id, err));
  EXPECT_FALSE(api_access_tokens_verify_bearer(token));
}

TEST(ApiAccessToken, ReplaceAllRestore) {
  api_access_tokens_clear();
  ApiAccessTokenEntry in[1];
  in[0].id = 3;
  in[0].label = "restore";
  in[0].token_hex = std::string(64, 'c');
  std::string err;
  ASSERT_TRUE(api_access_tokens_replace_all(in, 1, err));
  EXPECT_TRUE(api_access_tokens_verify_bearer(in[0].token_hex));
  EXPECT_FALSE(api_access_tokens_verify_bearer(std::string(64, 'd')));
}

TEST(ApiAccessToken, ReplaceAllRejectsInvalidSecret) {
  api_access_tokens_clear();
  ApiAccessTokenEntry in[1];
  in[0].id = 1;
  in[0].label = "bad";
  in[0].token_hex = "not-hex";
  std::string err;
  EXPECT_FALSE(api_access_tokens_replace_all(in, 1, err));
}

TEST(ApiAccessToken, ReplaceAllRejectsDuplicateId) {
  api_access_tokens_clear();
  ApiAccessTokenEntry in[2];
  in[0].id = 1;
  in[0].label = "a";
  in[0].token_hex = std::string(64, 'a');
  in[1].id = 1;
  in[1].label = "b";
  in[1].token_hex = std::string(64, 'b');
  std::string err;
  EXPECT_FALSE(api_access_tokens_replace_all(in, 2, err));
}

TEST(ApiAccessToken, ReplaceAllRejectsZeroId) {
  api_access_tokens_clear();
  ApiAccessTokenEntry in[1];
  in[0].id = 0;
  in[0].token_hex = std::string(64, 'f');
  std::string err;
  EXPECT_FALSE(api_access_tokens_replace_all(in, 1, err));
}

TEST(ApiAccessToken, ReplaceAllAppliesDefaultLabel) {
  api_access_tokens_clear();
  ApiAccessTokenEntry in[1];
  in[0].id = 2;
  in[0].label = "";
  in[0].token_hex = std::string(64, 'e');
  std::string err;
  ASSERT_TRUE(api_access_tokens_replace_all(in, 1, err));
  EXPECT_EQ(apiAccessTokens[0].label, "Token 2");
}

TEST(ApiAccessToken, MaxFourTokens) {
  api_access_tokens_clear();
  for (int i = 0; i < 4; i++) {
    std::string token;
    uint8_t id = 0;
    std::string labelOut;
    std::string err;
    ASSERT_TRUE(api_access_tokens_create("", token, id, labelOut, err));
  }
  std::string token;
  uint8_t id = 0;
  std::string labelOut;
  std::string err;
  EXPECT_FALSE(api_access_tokens_create("", token, id, labelOut, err));
}
