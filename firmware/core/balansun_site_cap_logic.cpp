#include "balansun_site_cap_logic.h"

int balansun_site_cap_logic_clamp_triac_open(int triac_open_percent, int estimated_routed_w, uint16_t max_routed_w) {
  if (max_routed_w == 0 || triac_open_percent <= 0) return triac_open_percent;
  if (estimated_routed_w <= 0) return triac_open_percent;
  if (estimated_routed_w <= max_routed_w) return triac_open_percent;
  const int scaled = (triac_open_percent * max_routed_w) / estimated_routed_w;
  if (scaled > 100) return 100;
  return scaled;
}
