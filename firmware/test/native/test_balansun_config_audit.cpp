#include <gtest/gtest.h>

#include "balansun_config_audit_keys.h"

TEST(BalansunConfigAudit, RedactsSecrets) {
  EXPECT_TRUE(balansun_config_audit_is_secret_key("mqtt_password"));
  EXPECT_TRUE(balansun_config_audit_is_secret_key("http_api_password"));
  EXPECT_FALSE(balansun_config_audit_is_secret_key("source"));
}
