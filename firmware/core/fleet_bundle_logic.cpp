#include "fleet_bundle_logic.h"

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <sstream>

#ifndef FLEET_BUNDLE_NATIVE_STUB
#include <mbedtls/md.h>
#endif

namespace {

bool icontains(const std::string &hay, const char *needle) {
  const std::string n(needle);
  if (n.empty() || hay.size() < n.size()) return false;
  for (size_t i = 0; i + n.size() <= hay.size(); i++) {
    bool ok = true;
    for (size_t j = 0; j < n.size(); j++) {
      if (std::tolower(static_cast<unsigned char>(hay[i + j])) !=
          std::tolower(static_cast<unsigned char>(n[j]))) {
        ok = false;
        break;
      }
    }
    if (ok) return true;
  }
  return false;
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

}  // namespace

bool fleet_bundle_contains_forbidden_secret(const std::string &json) {
  static const char *kForbidden[] = {"mqtt_password", "\"password\"", "enphase_password",
                                     "arduino_ota_password", "http_api_password", "fleet_trust_key",
                                     "api_access_token"};
  for (const char *f : kForbidden) {
    if (icontains(json, f)) return true;
  }
  return false;
}

std::string fleet_bundle_build_unsigned(const FleetBundle &parts) {
  std::ostringstream os;
  os << "{\"schema_version\":\"" << parts.schema_version << "\""
     << ",\"exported_at\":\"" << parts.exported_at << "\""
     << ",\"device_name\":\"" << parts.device_name << "\""
     << ",\"config\":" << (parts.config_json.empty() ? "{}" : parts.config_json)
     << ",\"actions\":" << (parts.actions_json.empty() ? "{}" : parts.actions_json) << "}";
  return os.str();
}

std::string fleet_bundle_merge_config(const std::string &patch_json, bool dry_run) {
  (void)patch_json;
  (void)dry_run;
  return "";
}

std::string fleet_bundle_merge_actions(const std::string &patch_json, bool dry_run) {
  (void)patch_json;
  (void)dry_run;
  return "";
}

std::string fleet_bundle_sign_hmac_sha256_hex(const std::string &payload, const std::string &key) {
  if (key.empty()) return "";
#ifdef FLEET_BUNDLE_NATIVE_STUB
  uint64_t h = 14695981039346656037ULL;
  const std::string mix = payload + "|" + key;
  for (unsigned char c : mix) {
    h ^= c;
    h *= 1099511628211ULL;
  }
  unsigned char out[8];
  for (int i = 0; i < 8; ++i) {
    out[i] = static_cast<unsigned char>((h >> (56 - 8 * i)) & 0xFF);
  }
  return bytes_to_hex(out, 8);
#else
  unsigned char out[32];
  const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (!info) return "";
  if (mbedtls_md_hmac(info, reinterpret_cast<const unsigned char *>(key.data()), key.size(),
                      reinterpret_cast<const unsigned char *>(payload.data()), payload.size(), out) != 0) {
    return "";
  }
  return bytes_to_hex(out, 32);
#endif
}

bool fleet_bundle_verify_hmac_sha256_hex(const std::string &payload, const std::string &key,
                                         const std::string &signature_hex) {
  if (key.empty() || signature_hex.empty()) return false;
  const std::string expect = fleet_bundle_sign_hmac_sha256_hex(payload, key);
  if (expect.length() != signature_hex.length()) return false;
  uint8_t diff = 0;
  for (size_t i = 0; i < expect.length(); i++) {
    diff |= static_cast<uint8_t>(expect[i] ^ signature_hex[i]);
  }
  return diff == 0;
}
