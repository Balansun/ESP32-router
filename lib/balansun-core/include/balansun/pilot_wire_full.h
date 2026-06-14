#pragma once

#include <balansun/pilot_wire.h>

/** ESP8266 R2-full wiring backend (HG and Arrêt on distinct paths). */
enum class PilotWireFullWiring : uint8_t {
  ThreeRelay = 0,
  TwoRelayMatrix,
};

struct PilotWireFullRelayMask {
  bool relay_full_wave = false;
  bool relay_half_negative = false;
  bool relay_half_positive = false;
};

/** Logical GPIO drive for R2-full wiring (3-relay direct; 2-relay DPDT selector). */
struct PilotWireFullGpioLevels {
  bool relay_full_wave = false;
  bool relay_half_negative = false;
  bool relay_half_positive = false;
  bool half_selector_active = false;
  bool half_select_arret = false;
};

PilotWireFullGpioLevels balansun_pilot_wire_full_gpio_levels(PilotWireFullRelayMask mask,
                                                             PilotWireFullWiring wiring);

PilotWireFullWiring balansun_pilot_wire_full_wiring_from_wire(const char *wire);
const char *balansun_pilot_wire_full_wiring_wire(PilotWireFullWiring wiring);

/** All four static orders supported on R2-full builds. */
bool balansun_pilot_wire_full_order_supported(PilotWireOrder order);

PilotWireFullRelayMask balansun_pilot_wire_full_relays_for_order(PilotWireOrder order,
                                                                 PilotWireFullWiring wiring);
