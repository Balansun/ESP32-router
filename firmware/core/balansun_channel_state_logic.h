#pragma once

#include "balansun_product_caps_logic.h"
#include "actions_logic.h"

#include <cstdint>

enum class BalansunChannelState : uint8_t {
  Disabled = 0,
  ScheduleOff,
  ScheduleOn,
  Surplus,
  Override,
  CapLimited,
  HeaterBackoff,
  Remote,
};

struct BalansunChannelStateInput {
  BalansunProductCaps caps{};
  int action_index = 0;
  bool is_remote_host = false;
  uint8_t regulation_mode = 0;
  uint8_t schedule_type = 0;
  uint8_t override_state = kActionOverrideAuto;
  bool cap_hit = false;
  bool heater_backoff = false;
};

struct BalansunChannelStateResult {
  BalansunChannelState state = BalansunChannelState::Disabled;
  const char *reason = "disabled";
};

BalansunChannelStateResult balansun_channel_state_compute(const BalansunChannelStateInput &in);

const char *balansun_channel_state_wire(BalansunChannelState state);
