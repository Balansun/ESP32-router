#include "balansun_config_daily_cap_logic.h"

void balansun_config_daily_cap_apply(uint32_t *dest, int nb_actions, const uint32_t *values,
                                  size_t value_count) {
  if (!dest || nb_actions <= 0 || !values || value_count == 0) return;
  const int n = nb_actions < (int)value_count ? nb_actions : (int)value_count;
  for (int i = 0; i < n; i++) {
    dest[i] = values[i];
  }
}
