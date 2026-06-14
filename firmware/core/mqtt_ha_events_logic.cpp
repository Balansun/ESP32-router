#include "mqtt_ha_events_logic.h"

#include <cstring>

void mqtt_ha_events_logic_detect(const MqttHaEventInput &in, MqttHaEventOutput *out) {
  if (!out) return;
  *out = MqttHaEventOutput{};
  if (in.surplus_active && !in.prev_surplus_active) out->surplus_started = true;
  if (!in.surplus_active && in.prev_surplus_active) out->surplus_ended = true;
  if (in.source_stale && !in.prev_source_stale) out->source_lost = true;
  if (in.site_cap_active && !in.prev_site_cap_active) out->triac_cap_hit = true;
  if (in.regulation_hunting && !in.prev_regulation_hunting) out->regulation_hunting_started = true;
  if (!in.vacation_active && in.prev_vacation_active) out->vacation_ended = true;
  if (in.action_cap_hit && !in.prev_action_cap_hit) out->action_cap_hit = true;
  if (in.linky_tariff && in.prev_linky_tariff && std::strcmp(in.linky_tariff, in.prev_linky_tariff) != 0) {
    out->linky_tariff_changed = true;
  }
}
