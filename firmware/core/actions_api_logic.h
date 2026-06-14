#pragma once

#include <cstdint>
#include <string>

struct ActionOverrideValidation {
  bool ok = false;
  std::string error;
  uint8_t override_state = 255;
};

/** idx in [0, max_actions); state: auto|on|off|triac_fixed
 *  max_temp_c: 0 disables cap; otherwise blocks triac_fixed at 100% when temperature_c > cap. */
ActionOverrideValidation actions_api_logic_validate_override(int idx, int max_actions, const char *state,
                                                               int triac_percent, float temperature_c,
                                                               int max_temp_c);

bool actions_api_logic_is_valid_action_index(int idx, int max_actions);
