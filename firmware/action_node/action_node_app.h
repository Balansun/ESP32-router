#pragma once

#include <balansun/action_node_control.h>
#include <balansun/pilot_wire_full.h>
#include <balansun/radiator_schedule.h>

#include <ArduinoJson.h>

#include "action_node_pins.h"

struct ActionNodePersistedConfig {
  PilotWireFullWiring wiring = PilotWireFullWiring::ThreeRelay;
  bool relay_active_low = true;
};

struct ActionNodeAppState {
  RadiatorScheduleConfig schedule{};
  ActionNodePersistedConfig config{};
  ActionNodeManualOverride manual{};
  ActionNodeRouterLatch router{};
  PilotWireOrder applied_order = PilotWireOrder::Confort;
  float temperature_c = 0.0f;
  bool temperature_ok = false;
  char wifi_ssid[33]{};
  char wifi_password[65]{};
};

void action_node_app_init(ActionNodeAppState &state);
void action_node_app_load(ActionNodeAppState &state);
void action_node_app_save_schedule(const ActionNodeAppState &state);
void action_node_app_save_config(const ActionNodeAppState &state);
void action_node_app_save_wifi(const ActionNodeAppState &state);

PilotWireFullWiring action_node_build_wiring(void);
bool action_node_wiring_matches_build(PilotWireFullWiring wiring);

void action_node_apply_relays(const PilotWireFullRelayMask &mask, PilotWireFullWiring wiring, bool active_low);
int action_node_wall_decihours(void);

void action_node_tick(ActionNodeAppState &state, unsigned long now_ms);
bool action_node_schedule_from_json(ActionNodeAppState &state, JsonObjectConst root, String &err);
void action_node_schedule_to_json(const ActionNodeAppState &state, JsonObject out);
void action_node_state_to_json(const ActionNodeAppState &state, JsonObject out, uint32_t now_ms);

bool action_node_set_command(ActionNodeAppState &state, const char *order_wire, int duration_sec,
                             unsigned long now_ms, String &err);
