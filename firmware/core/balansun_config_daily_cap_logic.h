#pragma once

#include <stddef.h>
#include <stdint.h>

/** Apply REST/MQTT `action_daily_cap_wh` array into per-action storage (partial update). */
void balansun_config_daily_cap_apply(uint32_t *dest, int nb_actions, const uint32_t *values,
                                  size_t value_count);
