#pragma once

#include <cstdint>
#include <string>
#include <vector>

/** SHA-256 hex digest of @p token (64 lowercase hex chars). */
std::string api_token_sha256_hex(const std::string &token);

/** True when @p label is empty or printable ASCII up to 24 chars. */
bool api_token_validate_label(const std::string &label, std::string &err);

/** Lowest unused id in 1..255, or 0 if none. */
uint8_t api_token_lowest_free_id(const std::vector<uint8_t> &used_ids);

/** Constant-time compare of two equal-length hex strings. */
bool api_token_hex_constant_time_eq(const std::string &a, const std::string &b);
