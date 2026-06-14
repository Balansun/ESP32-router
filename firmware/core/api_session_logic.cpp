#include "api_session_logic.h"

#include <cctype>
#include <cstring>

namespace {

constexpr const char kBearerPrefix[] = "Bearer ";

bool is_hex_lower(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

}  // namespace

bool api_logic_parse_bearer_token(const std::string &authHeader, std::string &tokenOut) {
  tokenOut.clear();
  if (authHeader.rfind(kBearerPrefix, 0) != 0) return false;
  tokenOut = authHeader.substr(strlen(kBearerPrefix));
  while (!tokenOut.empty() && tokenOut[0] == ' ') tokenOut.erase(0, 1);
  while (!tokenOut.empty() && tokenOut.back() == ' ') tokenOut.pop_back();
  return api_logic_is_valid_session_token_format(tokenOut);
}

bool api_logic_is_valid_session_token_format(const std::string &token) {
  if (token.size() != 64) return false;
  for (char c : token) {
    if (!is_hex_lower(c)) return false;
  }
  return true;
}

bool api_logic_session_token_constant_time_eq(const std::string &a, const std::string &b) {
  if (a.size() != b.size()) return false;
  unsigned char diff = 0;
  for (size_t i = 0; i < a.size(); i++) {
    diff |= (unsigned char)(a[i] ^ b[i]);
  }
  return diff == 0;
}

bool api_logic_password_constant_time_eq(const std::string &a, const std::string &b) {
  if (a.size() != b.size()) return false;
  unsigned char diff = 0;
  for (size_t i = 0; i < a.size(); i++) {
    diff |= (unsigned char)(a[i] ^ b[i]);
  }
  return diff == 0;
}
