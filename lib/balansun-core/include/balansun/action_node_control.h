#pragma once

#include <balansun/pilot_wire.h>
#include <balansun/pilot_wire_full.h>
#include <balansun/radiator_schedule.h>

#include <cstdint>

enum class ActionNodeControlMode : uint8_t {
  Auto = 0,
  Manual,
};

struct ActionNodeManualOverride {
  bool active = false;
  PilotWireOrder order = PilotWireOrder::Confort;
  uint32_t until_ms = 0;
};

struct ActionNodeRouterLatch {
  bool regulation_window = false;
  bool surplus_on = false;
};

struct ActionNodeRuntimeInput {
  RadiatorScheduleConfig schedule{};
  WarmActuatorConfig warm_cfg = balansun_warm_actuator_make_config(WarmHardwareSku::R2);
  PilotWireFullWiring wiring = PilotWireFullWiring::ThreeRelay;
  ActionNodeManualOverride manual{};
  ActionNodeRouterLatch router{};
  int wall_decihours = 0;
  float temperature_c = 0.0f;
  bool temperature_ok = false;
  uint32_t now_ms = 0;
};

struct ActionNodeRuntimeOutput {
  PilotWireOrder order = PilotWireOrder::Confort;
  ActionNodeControlMode control_mode = ActionNodeControlMode::Auto;
  PilotWireFullRelayMask relay_mask{};
};

void action_node_resolve(const ActionNodeRuntimeInput &in, ActionNodeRuntimeOutput &out);
