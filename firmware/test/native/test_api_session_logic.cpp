#include <gtest/gtest.h>

#include "api_session_logic.h"

TEST(ApiSessionLogic, ParseBearerToken) {
  std::string token;
  EXPECT_TRUE(api_logic_parse_bearer_token("Bearer " + std::string(64, 'a'), token));
  EXPECT_EQ(token.size(), 64u);
  EXPECT_FALSE(api_logic_parse_bearer_token("Basic abc", token));
  EXPECT_FALSE(api_logic_parse_bearer_token("Bearer short", token));
  EXPECT_FALSE(api_logic_parse_bearer_token("Bearer " + std::string(64, 'G'), token));
}

TEST(ApiSessionLogic, TokenFormat) {
  EXPECT_TRUE(api_logic_is_valid_session_token_format(std::string(64, '0')));
  EXPECT_FALSE(api_logic_is_valid_session_token_format("abc"));
}

TEST(ApiSessionLogic, ConstantTimeEq) {
  const std::string a(64, 'a');
  const std::string b(64, 'b');
  EXPECT_TRUE(api_logic_session_token_constant_time_eq(a, a));
  EXPECT_FALSE(api_logic_session_token_constant_time_eq(a, b));
  EXPECT_TRUE(api_logic_password_constant_time_eq("secret", "secret"));
  EXPECT_FALSE(api_logic_password_constant_time_eq("secret", "other"));
}

TEST(ApiSessionLogic, BearerTrimAndLengthMismatch) {
  std::string token;
  EXPECT_TRUE(api_logic_parse_bearer_token("Bearer   " + std::string(64, 'a') + "  ", token));
  const std::string full(64, 'a');
  const std::string short_token(63, 'a');
  EXPECT_FALSE(api_logic_session_token_constant_time_eq(full, short_token));
  EXPECT_FALSE(api_logic_password_constant_time_eq("a", "ab"));
}

TEST(ApiSessionLogic, RejectsNonHexAndEmptyToken) {
  std::string token;
  EXPECT_FALSE(api_logic_is_valid_session_token_format(std::string(64, 'Z')));
  EXPECT_FALSE(api_logic_parse_bearer_token("Bearer        ", token));
  EXPECT_TRUE(token.empty());
}

TEST(ApiSessionLogic, BearerTrimsLeadingSpacesInToken) {
  std::string token;
  EXPECT_TRUE(api_logic_parse_bearer_token("Bearer   " + std::string(64, 'b'), token));
  EXPECT_EQ(token, std::string(64, 'b'));
}

TEST(ApiSessionLogic, BearerPrefixCaseAndGarbage) {
  std::string token;
  EXPECT_FALSE(api_logic_parse_bearer_token("bearer " + std::string(64, 'a'), token));
  EXPECT_FALSE(api_logic_parse_bearer_token("Bearer", token));
  EXPECT_FALSE(api_logic_is_valid_session_token_format(std::string(63, 'a')));
}

TEST(ApiSessionLogic, RejectsUppercaseHexDigits) {
  std::string mixed(64, 'a');
  mixed[0] = 'A';
  EXPECT_FALSE(api_logic_is_valid_session_token_format(mixed));
  std::string token;
  EXPECT_FALSE(api_logic_parse_bearer_token("Bearer " + mixed, token));
}

TEST(ApiSessionLogic, PasswordEqDiffLength) {
  EXPECT_FALSE(api_logic_password_constant_time_eq("short", "longer"));
}

TEST(ApiSessionLogic, SessionTokenRejectsDigitNineAsHex) {
  std::string token(64, 'a');
  token[10] = '9';
  EXPECT_TRUE(api_logic_is_valid_session_token_format(token));
  token[11] = 'g';
  EXPECT_FALSE(api_logic_is_valid_session_token_format(token));
}
