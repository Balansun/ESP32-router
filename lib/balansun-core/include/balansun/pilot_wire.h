#pragma once

#include <cstdint>

enum class PilotWireOrder : uint8_t {
  Confort = 0,
  ConfortMinus1,
  ConfortMinus2,
  Eco,
  HorsGel,
  Arret,
};

enum class WarmHardwareSku : uint8_t {
  R1 = 0,
  R1D,
  R2,
  R2T,
  T,
  R1T,
};

struct PilotWireRelayMask {
  bool relay_full_wave = false;
  bool relay_half_wave = false;
};

bool balansun_pilot_wire_order_supported(PilotWireOrder order, WarmHardwareSku sku);
PilotWireOrder balansun_pilot_wire_clamp_order(PilotWireOrder order, WarmHardwareSku sku);
PilotWireRelayMask balansun_pilot_wire_relays_for_order(PilotWireOrder order, WarmHardwareSku sku,
                                                          bool diode_negative_is_hors_gel);

const char *balansun_pilot_wire_order_wire(PilotWireOrder order);
PilotWireOrder balansun_pilot_wire_order_from_wire(const char *wire);
