#include <gtest/gtest.h>

#include "fleet_bundle_logic.h"

TEST(FleetBundleLogic, RejectsForbiddenSecrets) {
  EXPECT_TRUE(fleet_bundle_contains_forbidden_secret("{\"mqtt_password\":\"x\"}"));
  EXPECT_FALSE(fleet_bundle_contains_forbidden_secret("{\"source\":\"Ext\"}"));
}

TEST(FleetBundleLogic, SignAndVerify) {
  const std::string payload = "{\"schema_version\":\"1\"}";
  const std::string key = "installer-secret";
  const std::string sig = fleet_bundle_sign_hmac_sha256_hex(payload, key);
  ASSERT_FALSE(sig.empty());
  EXPECT_TRUE(fleet_bundle_verify_hmac_sha256_hex(payload, key, sig));
  EXPECT_FALSE(fleet_bundle_verify_hmac_sha256_hex(payload, key, "deadbeef"));
}

TEST(FleetBundleLogic, ForbiddenSecretVariants) {
  EXPECT_TRUE(fleet_bundle_contains_forbidden_secret(R"({"enphase_password":"x"})"));
  EXPECT_TRUE(fleet_bundle_contains_forbidden_secret(R"({"arduino_ota_password":"x"})"));
  EXPECT_TRUE(fleet_bundle_contains_forbidden_secret(R"({"http_api_password":"x"})"));
  EXPECT_TRUE(fleet_bundle_contains_forbidden_secret(R"({"fleet_trust_key":"x"})"));
  EXPECT_TRUE(fleet_bundle_contains_forbidden_secret(R"({"password":"x"})"));
}

TEST(FleetBundleLogic, BuildUnsignedAndMergeStubs) {
  FleetBundle parts;
  parts.schema_version = "1";
  parts.exported_at = "2025-01-01";
  parts.device_name = "hz";
  const std::string json = fleet_bundle_build_unsigned(parts);
  EXPECT_NE(json.find("\"config\":{}"), std::string::npos);
  parts.config_json = R"({"a":1})";
  parts.actions_json = R"({"b":2})";
  const std::string full = fleet_bundle_build_unsigned(parts);
  EXPECT_NE(full.find(R"("config":{"a":1})"), std::string::npos);
  EXPECT_TRUE(fleet_bundle_merge_config("{}", true).empty());
  EXPECT_TRUE(fleet_bundle_merge_actions("{}", false).empty());
}

TEST(FleetBundleLogic, SignRejectsEmptyKey) {
  EXPECT_TRUE(fleet_bundle_sign_hmac_sha256_hex("payload", "").empty());
  EXPECT_FALSE(fleet_bundle_verify_hmac_sha256_hex("payload", "key", ""));
  EXPECT_FALSE(fleet_bundle_verify_hmac_sha256_hex("payload", "", "abcd"));
}

TEST(FleetBundleLogic, ForbiddenSecretCaseInsensitive) {
  EXPECT_TRUE(fleet_bundle_contains_forbidden_secret(R"({"MQTT_PASSWORD":"x"})"));
  EXPECT_TRUE(fleet_bundle_contains_forbidden_secret(R"({"x":"y","password":"z"})"));
}

TEST(FleetBundleLogic, VerifyLengthMismatch) {
  const std::string sig = fleet_bundle_sign_hmac_sha256_hex("p", "k");
  ASSERT_FALSE(sig.empty());
  EXPECT_FALSE(fleet_bundle_verify_hmac_sha256_hex("p", "k", sig + "ff"));
}
