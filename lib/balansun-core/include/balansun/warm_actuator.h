#pragma once

#include <balansun/pilot_wire.h>

#include <cstdint>

/** User-facing wiring profile (API `hardware_profile`). */
enum class WarmWiringProfile : uint8_t {
  R1 = 0,
  R1D,
  R2,
  R2T,
};

/** Hardware + wiring options for a Warm room node. */
struct WarmActuatorConfig {
  WarmHardwareSku sku = WarmHardwareSku::R1;
  bool relay_diode_installed = false;
  bool diode_negative_is_hors_gel = true;
};

inline WarmActuatorConfig balansun_warm_actuator_make_config(WarmHardwareSku sku) {
  WarmActuatorConfig cfg;
  cfg.sku = sku;
  return cfg;
}

WarmWiringProfile balansun_warm_wiring_profile(const WarmActuatorConfig &cfg);
const char *balansun_warm_wiring_profile_wire(WarmWiringProfile profile);
WarmWiringProfile balansun_warm_wiring_profile_from_wire(const char *wire);
WarmHardwareSku balansun_warm_pilot_sku(const WarmActuatorConfig &cfg);

struct WarmActuatorState {
  PilotWireOrder order = PilotWireOrder::Confort;
  int triac_open_percent = 0;
};

PilotWireRelayMask balansun_warm_actuator_relay_mask(const WarmActuatorState &state, const WarmActuatorConfig &cfg);

void balansun_warm_actuator_apply_command(WarmActuatorState &state, PilotWireOrder order, int triac_open_percent,
                                          const WarmActuatorConfig &cfg);
