#pragma once

#include <cstdint>
#include <string>

struct ApiAccessTokenEntry {
  uint8_t id = 0;
  std::string label;
  /** Secret bearer token (64 lowercase hex chars). */
  std::string token_hex;
};

constexpr int kApiAccessTokenMax = 4;

extern ApiAccessTokenEntry apiAccessTokens[kApiAccessTokenMax];
extern int apiAccessTokenCount;

void api_access_tokens_clear();
/** Load from EEPROM mirror (extension fields). */
void api_access_tokens_load(const ApiAccessTokenEntry *entries, int count);
/** Serialize current RAM tokens into extension fields. */
void api_access_tokens_to_eeprom(ApiAccessTokenEntry *entries, int &countOut);

bool api_access_tokens_find_by_id(uint8_t id, int &indexOut);
bool api_access_tokens_verify_bearer(const std::string &token);
/**
 * Create token: fills @p tokenOut (plaintext) and stores secret. Returns false if full or id exhausted.
 * @p label may be empty (default applied).
 */
bool api_access_tokens_create(const std::string &label, std::string &tokenOut, uint8_t &idOut,
                              std::string &labelOut, std::string &err);
bool api_access_tokens_revoke(uint8_t id, std::string &err);
/** Replace all tokens (backup restore). Each entry must have valid id, label, token_hex. */
bool api_access_tokens_replace_all(const ApiAccessTokenEntry *entries, int count, std::string &err);
