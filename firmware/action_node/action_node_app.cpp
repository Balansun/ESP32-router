#include "action_node_app.h"

#include <balansun/action_node_manual.h>
#include <balansun/action_node_time.h>
#include <balansun/pilot_wire_full.h>

#include <EEPROM.h>
#include <cstring>
#include <time.h>

namespace {

constexpr int kEepromSize = 4096;
constexpr uint32_t kEepromMagic = 0x414E3031UL;  // AN01

struct EepromBlob {
  uint32_t magic = 0;
  uint8_t schedule_count = 1;
  RadiatorSchedulePeriod periods[kRadiatorScheduleMaxPeriods]{};
  uint8_t wiring = 0;
  uint8_t relay_active_low = 1;
  char wifi_ssid[33]{};
  char wifi_password[65]{};
};

EepromBlob g_blob{};

void persist_blob() {
  EEPROM.put(0, g_blob);
  EEPROM.commit();
}

void load_blob() {
  EEPROM.get(0, g_blob);
  if (g_blob.magic != kEepromMagic) {
    g_blob = EepromBlob{};
    g_blob.magic = kEepromMagic;
    g_blob.wiring = static_cast<uint8_t>(PilotWireFullWiring::ThreeRelay);
    g_blob.relay_active_low = 1;
    g_blob.schedule_count = 1;
    g_blob.periods[0].kind = static_cast<uint8_t>(RadiatorPeriodKind::Regulation);
    g_blob.periods[0].period_end = kRadiatorScheduleTimeMax;
    persist_blob();
  }
}

PilotWireOrder order_from_period_json(JsonObjectConst p) {
  const char *order_wire = p["order"] | p["pilot_wire_order"] | "confort";
  return balansun_pilot_wire_order_from_wire(order_wire);
}

}  // namespace

PilotWireFullWiring action_node_build_wiring(void) {
#if BALANSUN_PILOT_WIRE_WIRING == R2_FULL_2RELAY
  return PilotWireFullWiring::TwoRelayMatrix;
#else
  return PilotWireFullWiring::ThreeRelay;
#endif
}

bool action_node_wiring_matches_build(PilotWireFullWiring wiring) { return wiring == action_node_build_wiring(); }

void action_node_app_init(ActionNodeAppState &state) {
  EEPROM.begin(kEepromSize);
  load_blob();
  action_node_app_load(state);
  pinMode(ACTION_NODE_PIN_FULL, OUTPUT);
  pinMode(ACTION_NODE_PIN_HALF_NEG, OUTPUT);
#if BALANSUN_PILOT_WIRE_WIRING == R2_FULL_3RELAY
  pinMode(ACTION_NODE_PIN_HALF_POS, OUTPUT);
#endif
  pinMode(ACTION_NODE_PIN_DS18, INPUT);
}

void action_node_app_load(ActionNodeAppState &state) {
  radiator_schedule_init_defaults(state.schedule);
  state.schedule.period_count = g_blob.schedule_count;
  for (int i = 0; i < kRadiatorScheduleMaxPeriods; i++) {
    state.schedule.periods[i] = g_blob.periods[i];
  }
  state.config.wiring = static_cast<PilotWireFullWiring>(g_blob.wiring);
  state.config.relay_active_low = g_blob.relay_active_low != 0;
  std::strncpy(state.wifi_ssid, g_blob.wifi_ssid, sizeof(state.wifi_ssid) - 1);
  std::strncpy(state.wifi_password, g_blob.wifi_password, sizeof(state.wifi_password) - 1);
}

void action_node_app_save_schedule(const ActionNodeAppState &state) {
  g_blob.schedule_count = state.schedule.period_count;
  for (int i = 0; i < kRadiatorScheduleMaxPeriods; i++) {
    g_blob.periods[i] = state.schedule.periods[i];
  }
  persist_blob();
}

void action_node_app_save_config(const ActionNodeAppState &state) {
  g_blob.wiring = static_cast<uint8_t>(state.config.wiring);
  g_blob.relay_active_low = state.config.relay_active_low ? 1 : 0;
  persist_blob();
}

void action_node_app_save_wifi(const ActionNodeAppState &state) {
  std::strncpy(g_blob.wifi_ssid, state.wifi_ssid, sizeof(g_blob.wifi_ssid) - 1);
  std::strncpy(g_blob.wifi_password, state.wifi_password, sizeof(g_blob.wifi_password) - 1);
  persist_blob();
}

static void write_relay(int pin, bool energized, bool active_low) {
  const int level = active_low ? (energized ? LOW : HIGH) : (energized ? HIGH : LOW);
  digitalWrite(pin, level);
}

void action_node_apply_relays(const PilotWireFullRelayMask &mask, PilotWireFullWiring wiring, bool active_low) {
  const PilotWireFullGpioLevels levels = balansun_pilot_wire_full_gpio_levels(mask, wiring);
  write_relay(ACTION_NODE_PIN_FULL, levels.relay_full_wave, active_low);
#if BALANSUN_PILOT_WIRE_WIRING == R2_FULL_3RELAY
  write_relay(ACTION_NODE_PIN_HALF_NEG, levels.relay_half_negative, active_low);
  write_relay(ACTION_NODE_PIN_HALF_POS, levels.relay_half_positive, active_low);
#else
  write_relay(ACTION_NODE_PIN_HALF_NEG, levels.half_selector_active, active_low);
  if (levels.half_selector_active) {
    const int level =
        active_low ? (levels.half_select_arret ? HIGH : LOW) : (levels.half_select_arret ? LOW : HIGH);
    digitalWrite(ACTION_NODE_PIN_HALF_NEG, level);
  }
#endif
}

int action_node_wall_decihours(void) {
  return balansun_wall_decihours_from_time(time(nullptr), 0);
}

void action_node_tick(ActionNodeAppState &state, unsigned long now_ms) {
  const uint32_t now = static_cast<uint32_t>(now_ms);
  if (action_node_manual_has_expired(state.manual.active, state.manual.until_ms, now)) {
    state.manual.active = false;
  }

  ActionNodeRuntimeInput in{};
  in.schedule = state.schedule;
  in.warm_cfg.sku = WarmHardwareSku::R2;
  in.wiring = state.config.wiring;
  in.manual = state.manual;
  in.router = state.router;
  in.wall_decihours = action_node_wall_decihours();
  in.temperature_c = state.temperature_c;
  in.temperature_ok = state.temperature_ok;
  in.now_ms = now;

  ActionNodeRuntimeOutput out{};
  action_node_resolve(in, out);
  if (out.order != state.applied_order) {
    state.applied_order = out.order;
  }
  action_node_apply_relays(out.relay_mask, state.config.wiring, state.config.relay_active_low);
}

bool action_node_schedule_from_json(ActionNodeAppState &state, JsonObjectConst root, String &err) {
  if (!root["periods"].is<JsonArrayConst>()) {
    err = "periods array required";
    return false;
  }
  JsonArrayConst periods = root["periods"].as<JsonArrayConst>();
  if (periods.size() < 1 || periods.size() > kRadiatorScheduleMaxPeriods) {
    err = "invalid period count";
    return false;
  }
  RadiatorScheduleConfig next{};
  radiator_schedule_init_defaults(next);
  next.period_count = static_cast<uint8_t>(periods.size());
  int prev_end = 0;
  for (size_t j = 0; j < periods.size(); j++) {
    JsonObjectConst p = periods[j].as<JsonObjectConst>();
    RadiatorSchedulePeriod &slot = next.periods[j];
    slot.period_start = prev_end;
    slot.period_end = p["hour_end"] | kRadiatorScheduleTimeMax;
    const char *kind_wire = p["kind"] | "regulation";
    if (std::strcmp(kind_wire, "fixed") == 0) {
      slot.kind = static_cast<uint8_t>(RadiatorPeriodKind::Fixed);
      slot.fixed_order = order_from_period_json(p);
    } else if (std::strcmp(kind_wire, "regulation") == 0) {
      slot.kind = static_cast<uint8_t>(RadiatorPeriodKind::Regulation);
    } else {
      err = "invalid period kind";
      return false;
    }
    slot.temp_inf_c = p["temp_inf_c"] | kRadiatorScheduleTempDisabled;
    slot.temp_sup_c = p["temp_sup_c"] | kRadiatorScheduleTempDisabled;
    prev_end = slot.period_end;
  }
  WarmActuatorConfig warm{.sku = WarmHardwareSku::R2};
  if (!radiator_schedule_valid(next, warm)) {
    err = "invalid radiator schedule";
    return false;
  }
  state.schedule = next;
  action_node_app_save_schedule(state);
  return true;
}

void action_node_schedule_to_json(const ActionNodeAppState &state, JsonObject out) {
  out["schema_version"] = 1;
  JsonArray periods = out["periods"].to<JsonArray>();
  const int n = radiator_schedule_clamp_period_count(state.schedule.period_count);
  for (int i = 0; i < n; i++) {
    const RadiatorSchedulePeriod &p = state.schedule.periods[i];
    JsonObject row = periods.add<JsonObject>();
    row["hour_end"] = p.period_end;
    if (p.kind == static_cast<uint8_t>(RadiatorPeriodKind::Fixed)) {
      row["kind"] = "fixed";
      row["order"] = balansun_pilot_wire_order_wire(p.fixed_order);
    } else {
      row["kind"] = "regulation";
    }
    if (p.temp_inf_c < kRadiatorScheduleTempDisabled) row["temp_inf_c"] = p.temp_inf_c;
    if (p.temp_sup_c < kRadiatorScheduleTempDisabled) row["temp_sup_c"] = p.temp_sup_c;
  }
}

void action_node_state_to_json(const ActionNodeAppState &state, JsonObject out, uint32_t now_ms) {
  out["pilot_wire_order"] = balansun_pilot_wire_order_wire(state.applied_order);
  const bool manual_live =
      action_node_manual_is_live(state.manual.active, state.manual.until_ms, now_ms);
  out["control_mode"] = action_node_control_mode_wire(action_node_control_mode_from_manual(manual_live));
  out["wiring_profile"] = balansun_pilot_wire_full_wiring_wire(state.config.wiring);
  JsonArray orders = out["supported_orders"].to<JsonArray>();
  orders.add("confort");
  orders.add("eco");
  orders.add("hors_gel");
  orders.add("arret");
  if (state.temperature_ok) {
    out["temperature_c"] = state.temperature_c;
  }
  out["temperature_ok"] = state.temperature_ok;
  const RadiatorSchedulePeriod *period = radiator_schedule_active_period(state.schedule, action_node_wall_decihours());
  if (period) {
    JsonObject active = out["active_period"].to<JsonObject>();
    active["kind"] = period->kind == static_cast<uint8_t>(RadiatorPeriodKind::Fixed) ? "fixed" : "regulation";
    if (period->kind == static_cast<uint8_t>(RadiatorPeriodKind::Fixed)) {
      active["order"] = balansun_pilot_wire_order_wire(period->fixed_order);
    }
  }
}

bool action_node_set_command(ActionNodeAppState &state, const char *order_wire, int duration_sec,
                             unsigned long now_ms, String &err) {
  if (!order_wire || !order_wire[0]) {
    err = "pilot_wire_order required";
    return false;
  }
  if (std::strcmp(order_wire, "auto") == 0) {
    state.manual.active = false;
    return true;
  }
  const PilotWireOrder order = balansun_pilot_wire_order_from_wire(order_wire);
  if (!balansun_pilot_wire_full_order_supported(order)) {
    err = "unsupported order";
    return false;
  }
  state.manual.active = true;
  state.manual.order = order;
  if (duration_sec <= 0) {
    state.manual.until_ms = 0;
  } else {
    state.manual.until_ms = static_cast<uint32_t>(now_ms + static_cast<unsigned long>(duration_sec) * 1000UL);
  }
  return true;
}
