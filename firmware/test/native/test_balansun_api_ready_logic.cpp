#include <gtest/gtest.h>

#include "balansun_api_ready_logic.h"

TEST(BalansunApiReadyLogic, BaseGateRequiresBootMeterAndTask) {
  EXPECT_FALSE(balansun_api_ready_logic_base(false, 100, true));
  EXPECT_FALSE(balansun_api_ready_logic_base(true, 0, true));
  EXPECT_FALSE(balansun_api_ready_logic_base(true, 100, false));
  EXPECT_TRUE(balansun_api_ready_logic_base(true, 100, true));
}

TEST(BalansunApiReadyLogic, PmqttBindingsGroups) {
  EXPECT_TRUE(balansun_pmqtt_bindings_ready_logic(PmqttBindingsPowerGroup::Signed, true, false, false,
                                               false));
  EXPECT_FALSE(balansun_pmqtt_bindings_ready_logic(PmqttBindingsPowerGroup::Signed, false, false,
                                                false, false));
  EXPECT_TRUE(balansun_pmqtt_bindings_ready_logic(PmqttBindingsPowerGroup::Split, false, true, true,
                                               false));
  EXPECT_FALSE(balansun_pmqtt_bindings_ready_logic(PmqttBindingsPowerGroup::Split, false, true, false,
                                                false));
  EXPECT_TRUE(balansun_pmqtt_bindings_ready_logic(PmqttBindingsPowerGroup::Snapshot, false, false,
                                               false, true));
}

TEST(BalansunApiReadyLogic, NoneAndUnknownGroupNotReady) {
  EXPECT_FALSE(balansun_pmqtt_bindings_ready_logic(PmqttBindingsPowerGroup::None, true, true, true,
                                                true));
  EXPECT_FALSE(balansun_pmqtt_bindings_ready_logic(static_cast<PmqttBindingsPowerGroup>(99), true,
                                                true, true, true));
  EXPECT_FALSE(balansun_pmqtt_bindings_ready_logic(PmqttBindingsPowerGroup::Split, false, true, false,
                                                false));
  EXPECT_FALSE(balansun_pmqtt_bindings_ready_logic(PmqttBindingsPowerGroup::Signed, false, false,
                                                false, false));
  EXPECT_FALSE(balansun_pmqtt_bindings_ready_logic(PmqttBindingsPowerGroup::Snapshot, false, false,
                                                false, false));
  EXPECT_TRUE(balansun_pmqtt_bindings_ready_logic(PmqttBindingsPowerGroup::Split, false, true, true,
                                               false));
  EXPECT_FALSE(balansun_pmqtt_bindings_ready_logic(PmqttBindingsPowerGroup::Split, false, false, true,
                                                false));
}
