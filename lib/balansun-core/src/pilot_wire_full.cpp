#include <balansun/pilot_wire_full.h>

#include <cstring>

namespace {

PilotWireFullRelayMask mask_open() { return {}; }

PilotWireFullRelayMask mask_eco() {
  PilotWireFullRelayMask m;
  m.relay_full_wave = true;
  return m;
}

PilotWireFullRelayMask mask_hors_gel(PilotWireFullWiring wiring) {
  PilotWireFullRelayMask m;
  if (wiring == PilotWireFullWiring::ThreeRelay) {
    m.relay_half_negative = true;
  } else {
    m.relay_half_negative = true;
  }
  return m;
}

PilotWireFullRelayMask mask_arret(PilotWireFullWiring wiring) {
  PilotWireFullRelayMask m;
  if (wiring == PilotWireFullWiring::ThreeRelay) {
    m.relay_half_positive = true;
  } else {
    m.relay_half_positive = true;
  }
  return m;
}

}  // namespace

PilotWireFullWiring balansun_pilot_wire_full_wiring_from_wire(const char *wire) {
  if (wire && std::strcmp(wire, "r2_full_2relay") == 0) {
    return PilotWireFullWiring::TwoRelayMatrix;
  }
  return PilotWireFullWiring::ThreeRelay;
}

const char *balansun_pilot_wire_full_wiring_wire(PilotWireFullWiring wiring) {
  switch (wiring) {
    case PilotWireFullWiring::TwoRelayMatrix:
      return "r2_full_2relay";
    case PilotWireFullWiring::ThreeRelay:
    default:
      return "r2_full_3relay";
  }
}

bool balansun_pilot_wire_full_order_supported(PilotWireOrder order) {
  return order == PilotWireOrder::Confort || order == PilotWireOrder::Eco ||
         order == PilotWireOrder::HorsGel || order == PilotWireOrder::Arret;
}

PilotWireFullRelayMask balansun_pilot_wire_full_relays_for_order(PilotWireOrder order,
                                                                  PilotWireFullWiring wiring) {
  if (!balansun_pilot_wire_full_order_supported(order)) {
    return mask_open();
  }
  switch (order) {
    case PilotWireOrder::Confort:
      return mask_open();
    case PilotWireOrder::Eco:
      return mask_eco();
    case PilotWireOrder::HorsGel:
      return mask_hors_gel(wiring);
    case PilotWireOrder::Arret:
      return mask_arret(wiring);
    default:
      return mask_open();
  }
}

PilotWireFullGpioLevels balansun_pilot_wire_full_gpio_levels(PilotWireFullRelayMask mask,
                                                             PilotWireFullWiring wiring) {
  PilotWireFullGpioLevels levels{};
  levels.relay_full_wave = mask.relay_full_wave;
  if (wiring == PilotWireFullWiring::ThreeRelay) {
    levels.relay_half_negative = mask.relay_half_negative;
    levels.relay_half_positive = mask.relay_half_positive;
    return levels;
  }
  levels.half_selector_active = mask.relay_half_negative || mask.relay_half_positive;
  levels.half_select_arret = mask.relay_half_positive;
  return levels;
}
