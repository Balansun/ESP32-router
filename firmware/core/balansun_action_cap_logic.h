#pragma once

#include <cstdint>

struct ActionCapInput {
  uint32_t daily_cap_wh = 0;
  uint32_t routed_wh_today = 0;
};

/** True when per-action daily routed energy cap is reached. */
bool balansun_action_cap_logic_is_hit(const ActionCapInput &in);
