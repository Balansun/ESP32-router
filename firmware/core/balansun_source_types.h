#pragma once

#include <cstdint>

/** Canonical measurement / ingress backends (wire string == EEPROM / HA select value). */
enum class SourceId : uint8_t {
  Unknown = 0,
  Analog,
  JsyMk194,
  JsyMk333,
  Linky,
  Enphase,
  SmartG,
  ShellyEm,
  ShellyPro,
  HomeW,
  NotDef,
  Pmqtt,
  VictronGx,
  BalansunPeer,
};
