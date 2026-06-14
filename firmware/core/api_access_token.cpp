#include "api_access_token.h"

#include "api_session_logic.h"
#include "api_token_logic.h"

#include <vector>

ApiAccessTokenEntry apiAccessTokens[kApiAccessTokenMax];
int apiAccessTokenCount = 0;

void api_access_tokens_clear() {
  apiAccessTokenCount = 0;
  for (int i = 0; i < kApiAccessTokenMax; i++) {
    apiAccessTokens[i] = ApiAccessTokenEntry{};
  }
}

void api_access_tokens_load(const ApiAccessTokenEntry *entries, int count) {
  api_access_tokens_clear();
  if (count < 0) count = 0;
  if (count > kApiAccessTokenMax) count = kApiAccessTokenMax;
  for (int i = 0; i < count; i++) {
    apiAccessTokens[i] = entries[i];
  }
  apiAccessTokenCount = count;
}

void api_access_tokens_to_eeprom(ApiAccessTokenEntry *entries, int &countOut) {
  countOut = apiAccessTokenCount;
  for (int i = 0; i < kApiAccessTokenMax; i++) {
    if (i < apiAccessTokenCount) {
      entries[i] = apiAccessTokens[i];
    } else {
      entries[i] = ApiAccessTokenEntry{};
    }
  }
}

bool api_access_tokens_find_by_id(uint8_t id, int &indexOut) {
  for (int i = 0; i < apiAccessTokenCount; i++) {
    if (apiAccessTokens[i].id == id) {
      indexOut = i;
      return true;
    }
  }
  return false;
}

bool api_access_tokens_verify_bearer(const std::string &token) {
  if (!api_logic_is_valid_session_token_format(token)) return false;
  for (int i = 0; i < apiAccessTokenCount; i++) {
    if (api_token_hex_constant_time_eq(token, apiAccessTokens[i].token_hex)) return true;
  }
  return false;
}

#ifndef API_TOKEN_NATIVE_STUB
#include <esp_random.h>
#endif

namespace {

std::string generate_token_hex64() {
  uint8_t bytes[32];
#ifdef API_TOKEN_NATIVE_STUB
  for (size_t i = 0; i < sizeof(bytes); i++) bytes[i] = static_cast<uint8_t>(i * 17 + 3);
#else
  esp_fill_random(bytes, sizeof(bytes));
#endif
  static const char hex[] = "0123456789abcdef";
  std::string out;
  out.reserve(64);
  for (size_t i = 0; i < sizeof(bytes); i++) {
    out.push_back(hex[(bytes[i] >> 4) & 0x0f]);
    out.push_back(hex[bytes[i] & 0x0f]);
  }
  return out;
}

}  // namespace

bool api_access_tokens_create(const std::string &label, std::string &tokenOut, uint8_t &idOut,
                              std::string &labelOut, std::string &err) {
  err.clear();
  if (apiAccessTokenCount >= kApiAccessTokenMax) {
    err = "maximum number of tokens reached";
    return false;
  }
  std::string labelNorm = label;
  if (!api_token_validate_label(labelNorm, err)) return false;

  std::vector<uint8_t> used;
  used.reserve(static_cast<size_t>(apiAccessTokenCount));
  for (int i = 0; i < apiAccessTokenCount; i++) used.push_back(apiAccessTokens[i].id);
  const uint8_t id = api_token_lowest_free_id(used);
  if (id == 0) {
    err = "no free token id";
    return false;
  }

  tokenOut = generate_token_hex64();
  if (!api_logic_is_valid_session_token_format(tokenOut)) {
    err = "failed to generate token";
    return false;
  }

  if (labelNorm.empty()) {
    labelNorm = "Token " + std::to_string(static_cast<int>(id));
  }
  ApiAccessTokenEntry &e = apiAccessTokens[apiAccessTokenCount++];
  e.id = id;
  e.label = labelNorm;
  e.token_hex = tokenOut;
  idOut = id;
  labelOut = labelNorm;
  return true;
}

bool api_access_tokens_revoke(uint8_t id, std::string &err) {
  err.clear();
  int idx = -1;
  if (!api_access_tokens_find_by_id(id, idx)) {
    err = "token not found";
    return false;
  }
  for (int i = idx; i < apiAccessTokenCount - 1; i++) {
    apiAccessTokens[i] = apiAccessTokens[i + 1];
  }
  apiAccessTokenCount--;
  apiAccessTokens[apiAccessTokenCount] = ApiAccessTokenEntry{};
  return true;
}

bool api_access_tokens_replace_all(const ApiAccessTokenEntry *entries, int count, std::string &err) {
  err.clear();
  if (count < 0 || count > kApiAccessTokenMax) {
    err = "invalid token count";
    return false;
  }
  std::vector<uint8_t> seen;
  seen.reserve(static_cast<size_t>(count));
  for (int i = 0; i < count; i++) {
    const ApiAccessTokenEntry &in = entries[i];
    if (in.id == 0) {
      err = "invalid token id";
      return false;
    }
    for (uint8_t s : seen) {
      if (s == in.id) {
        err = "duplicate token id";
        return false;
      }
    }
    seen.push_back(in.id);
    if (!api_logic_is_valid_session_token_format(in.token_hex)) {
      err = "invalid token secret";
      return false;
    }
    std::string labelNorm = in.label;
    if (!api_token_validate_label(labelNorm, err)) return false;
    if (labelNorm.empty()) {
      labelNorm = "Token " + std::to_string(static_cast<int>(in.id));
    }
  }
  api_access_tokens_clear();
  for (int i = 0; i < count; i++) {
    ApiAccessTokenEntry e = entries[i];
    if (e.label.empty()) {
      e.label = "Token " + std::to_string(static_cast<int>(e.id));
    }
    apiAccessTokens[i] = e;
  }
  apiAccessTokenCount = count;
  return true;
}
