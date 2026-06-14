#pragma once

#include <cstdint>

struct MqttHaEventInput {
  bool surplus_active = false;
  bool prev_surplus_active = false;
  bool source_stale = false;
  bool prev_source_stale = false;
  bool site_cap_active = false;
  bool prev_site_cap_active = false;
  bool regulation_hunting = false;
  bool prev_regulation_hunting = false;
  bool vacation_active = false;
  bool prev_vacation_active = false;
  bool action_cap_hit = false;
  bool prev_action_cap_hit = false;
  const char *linky_tariff = "";
  const char *prev_linky_tariff = "";
};

struct MqttHaEventOutput {
  bool surplus_started = false;
  bool surplus_ended = false;
  bool source_lost = false;
  bool triac_cap_hit = false;
  bool linky_tariff_changed = false;
  bool regulation_hunting_started = false;
  bool vacation_ended = false;
  bool action_cap_hit = false;
};

void mqtt_ha_events_logic_detect(const MqttHaEventInput &in, MqttHaEventOutput *out);
