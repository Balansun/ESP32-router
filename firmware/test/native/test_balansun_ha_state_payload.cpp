#include <gtest/gtest.h>

#include "balansun_source_health_logic.h"

TEST(BalansunHaStatePayload, stale_threshold_matches_mqtt) {
  EXPECT_FALSE(balansun_source_health_logic_is_stale(50));
  EXPECT_TRUE(balansun_source_health_logic_is_stale(49));
}
