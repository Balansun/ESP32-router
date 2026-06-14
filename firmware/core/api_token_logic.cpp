#include "api_token_logic.h"

#include <cctype>

#ifndef API_TOKEN_NATIVE_STUB
#include <mbedtls/md.h>
#endif

namespace {

bool is_printable_ascii(const std::string &s, size_t maxLen) {
  if (s.length() > maxLen) return false;
  for (unsigned char c : s) {
    if (c < 32 || c > 126) return false;
  }
  return true;
}

std::string bytes_to_hex(const unsigned char *data, size_t len) {
  static const char *kHex = "0123456789abcdef";
  std::string out;
  out.reserve(len * 2);
  for (size_t i = 0; i < len; i++) {
    out.push_back(kHex[(data[i] >> 4) & 0xF]);
    out.push_back(kHex[data[i] & 0xF]);
  }
  return out;
}

#ifdef API_TOKEN_NATIVE_STUB
std::string stub_sha256_hex(const std::string &token) {
  uint64_t h = 14695981039346656037ULL;
  for (unsigned char c : token) {
    h ^= c;
    h *= 1099511628211ULL;
  }
  unsigned char out[32];
  for (int i = 0; i < 32; ++i) {
    out[i] = static_cast<unsigned char>((h >> ((i % 8) * 8)) & 0xFF);
    h = h * 6364136223846793005ULL + 1;
  }
  return bytes_to_hex(out, 32);
}
#endif

}  // namespace

std::string api_token_sha256_hex(const std::string &token) {
  if (token.empty()) return "";
#ifdef API_TOKEN_NATIVE_STUB
  return stub_sha256_hex(token);
#else
  unsigned char out[32];
  const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (!info) return "";
  if (mbedtls_md(info, reinterpret_cast<const unsigned char *>(token.data()), token.size(), out) != 0) {
    return "";
  }
  return bytes_to_hex(out, 32);
#endif
}

bool api_token_validate_label(const std::string &label, std::string &err) {
  err.clear();
  if (label.empty()) return true;
  if (!is_printable_ascii(label, 24)) {
    err = "label must be printable ASCII (max 24 chars)";
    return false;
  }
  return true;
}

uint8_t api_token_lowest_free_id(const std::vector<uint8_t> &used_ids) {
  for (uint16_t id = 1; id <= 255; id++) {
    bool taken = false;
    for (uint8_t u : used_ids) {
      if (u == static_cast<uint8_t>(id)) {
        taken = true;
        break;
      }
    }
    if (!taken) return static_cast<uint8_t>(id);
  }
  return 0;
}

bool api_token_hex_constant_time_eq(const std::string &a, const std::string &b) {
  if (a.size() != b.size()) return false;
  unsigned char diff = 0;
  for (size_t i = 0; i < a.size(); i++) {
    diff |= static_cast<unsigned char>(a[i] ^ b[i]);
  }
  return diff == 0;
}
