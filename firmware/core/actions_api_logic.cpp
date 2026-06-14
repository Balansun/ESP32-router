#include "actions_api_logic.h"

#include "actions_logic.h"

#include <cstring>

ActionOverrideValidation actions_api_logic_validate_override(int idx, int max_actions, const char *state,
                                                               int triac_percent, float temperature_c,
                                                               int max_temp_c) {
  ActionOverrideValidation out;
  if (!actions_api_logic_is_valid_action_index(idx, max_actions)) {
    out.error = "action index out of range";
    return out;
  }
  if (!state) {
    out.error = "state must be auto, on, off, or triac_fixed";
    return out;
  }
  if (strcasecmp(state, "auto") == 0) {
    out.override_state = kActionOverrideAuto;
  } else if (strcasecmp(state, "on") == 0) {
    out.override_state = kActionOverrideOn;
  } else if (strcasecmp(state, "off") == 0) {
    out.override_state = kActionOverrideOff;
  } else if (strcasecmp(state, "triac_fixed") == 0) {
    out.override_state = kActionOverrideTriacFixed;
  } else {
    out.error = "state must be auto, on, off, or triac_fixed";
    return out;
  }
  if (out.override_state == kActionOverrideTriacFixed && idx != 0) {
    out.error = "triac_fixed is only valid for action 0";
    return out;
  }
  if (out.override_state == kActionOverrideTriacFixed && (triac_percent < 0 || triac_percent > 100)) {
    out.error = "triac_open_percent must be 0..100";
    return out;
  }
  if (out.override_state == kActionOverrideTriacFixed && triac_percent >= 100 && max_temp_c > 0 &&
      temperature_c > static_cast<float>(max_temp_c)) {
    out.error = "triac_fixed at 100% blocked above temperature cap";
    return out;
  }
  out.ok = true;
  return out;
}

bool actions_api_logic_is_valid_action_index(int idx, int max_actions) {
  return idx >= 0 && idx < max_actions;
}
