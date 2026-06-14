#include <gtest/gtest.h>

#include "actions_logic.h"
#include "balansun_channel_state_logic.h"
#include "balansun_product_profile.h"
#include "balansun_regulation_modes.h"

TEST(BalansunChannelState, RemoteHost) {
  BalansunChannelStateInput in{};
  in.is_remote_host = true;
  const auto r = balansun_channel_state_compute(in);
  EXPECT_EQ(r.state, BalansunChannelState::Remote);
  EXPECT_STREQ(r.reason, "remote_host");
  EXPECT_STREQ(balansun_channel_state_wire(r.state), "remote");
}

TEST(BalansunChannelState, DisabledInactive) {
  BalansunChannelStateInput in{};
  in.regulation_mode = kModeInactif;
  const auto r = balansun_channel_state_compute(in);
  EXPECT_EQ(r.state, BalansunChannelState::Disabled);
  EXPECT_STREQ(balansun_channel_state_wire(r.state), "disabled");
}

TEST(BalansunChannelState, HeaterBackoffTriacOnly) {
  BalansunChannelStateInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  in.regulation_mode = kModeDemisinus;
  in.action_index = 0;
  in.heater_backoff = true;
  const auto r = balansun_channel_state_compute(in);
  EXPECT_EQ(r.state, BalansunChannelState::HeaterBackoff);
  EXPECT_STREQ(balansun_channel_state_wire(r.state), "heater_backoff");
}

TEST(BalansunChannelState, CapOverrideScheduleSurplus) {
  BalansunChannelStateInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  in.regulation_mode = kModeDemisinus;
  in.cap_hit = true;
  EXPECT_EQ(balansun_channel_state_compute(in).state, BalansunChannelState::CapLimited);
  EXPECT_STREQ(balansun_channel_state_wire(BalansunChannelState::CapLimited), "cap_limited");
  in.cap_hit = false;
  in.override_state = kActionOverrideOn;
  EXPECT_EQ(balansun_channel_state_compute(in).state, BalansunChannelState::Override);
  EXPECT_STREQ(balansun_channel_state_wire(BalansunChannelState::Override), "override");
  in.override_state = kActionOverrideAuto;
  in.schedule_type = 0;
  EXPECT_EQ(balansun_channel_state_compute(in).state, BalansunChannelState::ScheduleOff);
  EXPECT_STREQ(balansun_channel_state_wire(BalansunChannelState::ScheduleOff), "schedule_off");
  in.schedule_type = 2;
  EXPECT_EQ(balansun_channel_state_compute(in).state, BalansunChannelState::ScheduleOn);
  EXPECT_STREQ(balansun_channel_state_wire(BalansunChannelState::ScheduleOn), "schedule_on");
  in.schedule_type = 4;
  EXPECT_EQ(balansun_channel_state_compute(in).state, BalansunChannelState::Surplus);
  EXPECT_STREQ(balansun_channel_state_wire(BalansunChannelState::Surplus), "surplus");
}

TEST(BalansunChannelState, HeaterBackoffSkipsWithoutTriacCap) {
  BalansunChannelStateInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_METER_ONLY);
  in.regulation_mode = kModeDemisinus;
  in.action_index = 0;
  in.heater_backoff = true;
  in.schedule_type = 3;
  EXPECT_EQ(balansun_channel_state_compute(in).state, BalansunChannelState::Surplus);
}

TEST(BalansunChannelState, ScheduleTypeOneIsOff) {
  BalansunChannelStateInput in{};
  in.regulation_mode = kModeDemisinus;
  in.schedule_type = 1;
  EXPECT_EQ(balansun_channel_state_compute(in).state, BalansunChannelState::ScheduleOff);
}

TEST(BalansunChannelState, InactiveSkipsHeaterBackoff) {
  BalansunChannelStateInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  in.regulation_mode = kModeInactif;
  in.heater_backoff = true;
  in.action_index = 0;
  EXPECT_EQ(balansun_channel_state_compute(in).state, BalansunChannelState::Disabled);
}

TEST(BalansunChannelState, HeaterBackoffOnlyChannelZero) {
  BalansunChannelStateInput in{};
  in.caps = balansun_product_caps_for_profile(BALANSUN_PRODUCT_FULL_ROUTER);
  in.regulation_mode = kModeDemisinus;
  in.heater_backoff = true;
  in.action_index = 1;
  in.schedule_type = 3;
  EXPECT_EQ(balansun_channel_state_compute(in).state, BalansunChannelState::Surplus);
}
