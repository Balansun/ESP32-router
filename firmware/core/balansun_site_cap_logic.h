#pragma once

#include <cstdint>

/** Scale triac open percent down so estimated routed power stays at or below max_routed_w. */
int balansun_site_cap_logic_clamp_triac_open(int triac_open_percent, int estimated_routed_w, uint16_t max_routed_w);
