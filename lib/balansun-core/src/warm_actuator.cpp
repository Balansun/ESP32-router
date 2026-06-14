#include <balansun/warm_actuator.h>

#include <cstring>

WarmWiringProfile balansun_warm_wiring_profile(const WarmActuatorConfig &cfg) {
  switch (cfg.sku) {
    case WarmHardwareSku::R2:
      return WarmWiringProfile::R2;
    case WarmHardwareSku::R2T:
      return WarmWiringProfile::R2T;
    case WarmHardwareSku::R1:
    case WarmHardwareSku::R1D:
    case WarmHardwareSku::T:
    case WarmHardwareSku::R1T:
    default:
      return cfg.relay_diode_installed ? WarmWiringProfile::R1D : WarmWiringProfile::R1;
  }
}

const char *balansun_warm_wiring_profile_wire(WarmWiringProfile profile) {
  switch (profile) {
    case WarmWiringProfile::R1:
      return "r1";
    case WarmWiringProfile::R1D:
      return "r1d";
    case WarmWiringProfile::R2:
      return "r2";
    case WarmWiringProfile::R2T:
      return "r2t";
  }
  return "r1";
}

WarmWiringProfile balansun_warm_wiring_profile_from_wire(const char *wire) {
  if (!wire || !wire[0]) return WarmWiringProfile::R1;
  if (std::strcmp(wire, "r1d") == 0) return WarmWiringProfile::R1D;
  if (std::strcmp(wire, "r2") == 0) return WarmWiringProfile::R2;
  if (std::strcmp(wire, "r2t") == 0) return WarmWiringProfile::R2T;
  return WarmWiringProfile::R1;
}

WarmHardwareSku balansun_warm_pilot_sku(const WarmActuatorConfig &cfg) {
  switch (cfg.sku) {
    case WarmHardwareSku::R2:
    case WarmHardwareSku::R2T:
      return cfg.sku;
    case WarmHardwareSku::R1:
    case WarmHardwareSku::R1D:
    case WarmHardwareSku::T:
    case WarmHardwareSku::R1T:
      return cfg.relay_diode_installed ? WarmHardwareSku::R1D : WarmHardwareSku::R1;
    default:
      return cfg.sku;
  }
}

PilotWireRelayMask balansun_warm_actuator_relay_mask(const WarmActuatorState &state, const WarmActuatorConfig &cfg) {
  return balansun_pilot_wire_relays_for_order(state.order, balansun_warm_pilot_sku(cfg),
                                              cfg.diode_negative_is_hors_gel);
}

void balansun_warm_actuator_apply_command(WarmActuatorState &state, PilotWireOrder order, int triac_open_percent,
                                          const WarmActuatorConfig &cfg) {
  state.order = balansun_pilot_wire_clamp_order(order, balansun_warm_pilot_sku(cfg));
  if (triac_open_percent < 0) triac_open_percent = 0;
  if (triac_open_percent > 100) triac_open_percent = 100;
  state.triac_open_percent = triac_open_percent;
}
