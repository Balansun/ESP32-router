#include <balansun/pilot_wire.h>

#include <cstring>

namespace {

PilotWireRelayMask relays_open() { return {}; }

PilotWireRelayMask relays_eco() {
  PilotWireRelayMask m;
  m.relay_full_wave = true;
  return m;
}

PilotWireRelayMask relays_half_wave() {
  PilotWireRelayMask m;
  m.relay_half_wave = true;
  return m;
}

PilotWireRelayMask relays_r1d_diode_path() {
  PilotWireRelayMask m;
  m.relay_full_wave = true;
  return m;
}

}  // namespace

bool balansun_pilot_wire_order_supported(PilotWireOrder order, WarmHardwareSku sku) {
  switch (sku) {
    case WarmHardwareSku::R1:
    case WarmHardwareSku::T:
      return order == PilotWireOrder::Confort || order == PilotWireOrder::Eco;
    case WarmHardwareSku::R1D:
      return order == PilotWireOrder::Confort || order == PilotWireOrder::HorsGel ||
             order == PilotWireOrder::Arret;
    case WarmHardwareSku::R2:
    case WarmHardwareSku::R1T:
      return order == PilotWireOrder::Confort || order == PilotWireOrder::Eco ||
             order == PilotWireOrder::HorsGel || order == PilotWireOrder::Arret;
    case WarmHardwareSku::R2T:
      return true;
  }
  return false;
}

PilotWireOrder balansun_pilot_wire_clamp_order(PilotWireOrder order, WarmHardwareSku sku) {
  if (balansun_pilot_wire_order_supported(order, sku)) return order;
  return PilotWireOrder::Confort;
}

PilotWireRelayMask balansun_pilot_wire_relays_for_order(PilotWireOrder order, WarmHardwareSku sku,
                                                          bool diode_negative_is_hors_gel) {
  order = balansun_pilot_wire_clamp_order(order, sku);

  switch (order) {
    case PilotWireOrder::Confort:
    case PilotWireOrder::ConfortMinus1:
    case PilotWireOrder::ConfortMinus2:
      return relays_open();
    case PilotWireOrder::Eco:
      return relays_eco();
    case PilotWireOrder::HorsGel:
      if (sku == WarmHardwareSku::R1D) {
        if (diode_negative_is_hors_gel) return relays_r1d_diode_path();
      } else if (sku == WarmHardwareSku::R2 || sku == WarmHardwareSku::R2T || sku == WarmHardwareSku::R1T) {
        if (diode_negative_is_hors_gel) return relays_half_wave();
      }
      return relays_open();
    case PilotWireOrder::Arret:
      if (sku == WarmHardwareSku::R1D) {
        if (!diode_negative_is_hors_gel) return relays_r1d_diode_path();
      } else if (sku == WarmHardwareSku::R2 || sku == WarmHardwareSku::R2T || sku == WarmHardwareSku::R1T) {
        if (!diode_negative_is_hors_gel) return relays_half_wave();
      }
      return relays_open();
  }
  return relays_open();
}

const char *balansun_pilot_wire_order_wire(PilotWireOrder order) {
  switch (order) {
    case PilotWireOrder::Confort:
      return "confort";
    case PilotWireOrder::ConfortMinus1:
      return "confort_minus_1";
    case PilotWireOrder::ConfortMinus2:
      return "confort_minus_2";
    case PilotWireOrder::Eco:
      return "eco";
    case PilotWireOrder::HorsGel:
      return "hors_gel";
    case PilotWireOrder::Arret:
      return "arret";
  }
  return "confort";
}

PilotWireOrder balansun_pilot_wire_order_from_wire(const char *wire) {
  if (!wire || !wire[0]) return PilotWireOrder::Confort;
  if (std::strcmp(wire, "eco") == 0) return PilotWireOrder::Eco;
  if (std::strcmp(wire, "confort_minus_1") == 0) return PilotWireOrder::ConfortMinus1;
  if (std::strcmp(wire, "confort_minus_2") == 0) return PilotWireOrder::ConfortMinus2;
  if (std::strcmp(wire, "hors_gel") == 0) return PilotWireOrder::HorsGel;
  if (std::strcmp(wire, "arret") == 0) return PilotWireOrder::Arret;
  if (std::strcmp(wire, "auto") == 0) return PilotWireOrder::Confort;
  return PilotWireOrder::Confort;
}
