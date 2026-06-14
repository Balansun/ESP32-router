#include "balansun_channel_state_logic.h"

#include "balansun_regulation_modes.h"

BalansunChannelStateResult balansun_channel_state_compute(const BalansunChannelStateInput &in) {
  BalansunChannelStateResult out{};
  if (in.is_remote_host) {
    out.state = BalansunChannelState::Remote;
    out.reason = "remote_host";
    return out;
  }
  if (in.regulation_mode == kModeInactif) {
    out.state = BalansunChannelState::Disabled;
    out.reason = "actif_inactif";
    return out;
  }
  if (in.heater_backoff && in.action_index == 0 && balansun_product_caps_has(in.caps, BalansunCap::TriacDimming)) {
    out.state = BalansunChannelState::HeaterBackoff;
    out.reason = "heater_backoff";
    return out;
  }
  if (in.cap_hit) {
    out.state = BalansunChannelState::CapLimited;
    out.reason = "daily_cap";
    return out;
  }
  if (in.override_state != kActionOverrideAuto) {
    out.state = BalansunChannelState::Override;
    out.reason = "manual_override";
    return out;
  }
  if (in.schedule_type <= 1) {
    out.state = BalansunChannelState::ScheduleOff;
    out.reason = "schedule_off";
    return out;
  }
  if (in.schedule_type == 2) {
    out.state = BalansunChannelState::ScheduleOn;
    out.reason = "schedule_on";
    return out;
  }
  out.state = BalansunChannelState::Surplus;
  out.reason = "surplus_regulation";
  return out;
}

const char *balansun_channel_state_wire(BalansunChannelState state) {
  switch (state) {
    case BalansunChannelState::ScheduleOff:
      return "schedule_off";
    case BalansunChannelState::ScheduleOn:
      return "schedule_on";
    case BalansunChannelState::Surplus:
      return "surplus";
    case BalansunChannelState::Override:
      return "override";
    case BalansunChannelState::CapLimited:
      return "cap_limited";
    case BalansunChannelState::HeaterBackoff:
      return "heater_backoff";
    case BalansunChannelState::Remote:
      return "remote";
    case BalansunChannelState::Disabled:
    default:
      return "disabled";
  }
}
