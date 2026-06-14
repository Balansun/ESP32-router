#include "balansun_action_cap_logic.h"

bool balansun_action_cap_logic_is_hit(const ActionCapInput &in) {
  if (in.daily_cap_wh == 0) return false;
  return in.routed_wh_today >= in.daily_cap_wh;
}
