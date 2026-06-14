#pragma once

#include <string>
#include <vector>

struct FleetBundle {
  std::string schema_version = "1";
  std::string exported_at;
  std::string device_name;
  std::string config_json;
  std::string actions_json;
  std::string signature;
};

/** Keys that must never appear in export (case-sensitive substring check on serialized JSON). */
bool fleet_bundle_contains_forbidden_secret(const std::string &json);

/** Build unsigned bundle JSON (no signature field). */
std::string fleet_bundle_build_unsigned(const FleetBundle &parts);

/** Apply merge from config/actions JSON strings; returns error message or empty on success. */
std::string fleet_bundle_merge_config(const std::string &patch_json, bool dry_run);

std::string fleet_bundle_merge_actions(const std::string &patch_json, bool dry_run);

/** HMAC-SHA256 hex signature (native: stub key; device uses mbedtls). */
std::string fleet_bundle_sign_hmac_sha256_hex(const std::string &payload, const std::string &key);

bool fleet_bundle_verify_hmac_sha256_hex(const std::string &payload, const std::string &key,
                                         const std::string &signature_hex);
