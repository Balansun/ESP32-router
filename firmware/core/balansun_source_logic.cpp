/*
 * balansun_source_logic.cpp — Wire label table and capability predicates (native tests).
 */
#include "balansun_source_logic.h"

#include "balansun_meter_sources_enable.h"
#include "balansun_product_profile.h"

#include <cstring>

struct SourceWireRow {
  SourceId id;
  const char *wire;
};

static const char kWireOutOfRange[] = "";

static const SourceWireRow kWireTable[] = {
#if BALANSUN_ENABLE_SOURCE_JSY_MK194
    {SourceId::JsyMk194, "JsyMk194"},
#endif
#if BALANSUN_ENABLE_SOURCE_JSY_MK333
    {SourceId::JsyMk333, "JsyMk333"},
#endif
#if BALANSUN_ENABLE_SOURCE_ANALOG
    {SourceId::Analog, "Analog"},
#endif
#if BALANSUN_ENABLE_SOURCE_LINKY
    {SourceId::Linky, "Linky"},
#endif
#if BALANSUN_ENABLE_SOURCE_ENPHASE
    {SourceId::Enphase, "Enphase"},
#endif
#if BALANSUN_ENABLE_SOURCE_SHELLY_EM
    {SourceId::ShellyEm, "ShellyEm"},
#endif
#if BALANSUN_ENABLE_SOURCE_SHELLY_PRO
    {SourceId::ShellyPro, "ShellyPro"},
#endif
#if BALANSUN_ENABLE_SOURCE_SMARTG
    {SourceId::SmartG, "SmartG"},
#endif
#if BALANSUN_ENABLE_SOURCE_HOMEW
    {SourceId::HomeW, "HomeW"},
#endif
#if BALANSUN_ENABLE_SOURCE_PMqtt
    {SourceId::Pmqtt, "Pmqtt"},
#endif
#if BALANSUN_ENABLE_SOURCE_NOTDEF
    {SourceId::NotDef, "NotDef"},
#endif
#if BALANSUN_ENABLE_SOURCE_EXT
    {SourceId::BalansunPeer, "BalansunPeer"},
#endif
};

size_t balansun_source_logic_registry_count() { return sizeof(kWireTable) / sizeof(kWireTable[0]); }

const char *balansun_source_logic_wire_at(size_t i) {
  if (i >= balansun_source_logic_registry_count()) return kWireOutOfRange;
  return kWireTable[i].wire;
}

const char *balansun_source_logic_normalize_wire_cstr(const char *wire) { return wire ? wire : ""; }

SourceId balansun_source_logic_parse_wire(const char *wire) {
  if (!wire) return SourceId::Unknown;
  for (const auto &row : kWireTable) {
    if (strcmp(wire, row.wire) == 0) return row.id;
  }
  return SourceId::Unknown;
}

SourceId balansun_source_logic_effective_id(SourceId active, const char *source_data_wire) {
  if (active == SourceId::BalansunPeer) return balansun_source_logic_parse_wire(source_data_wire);
  return active;
}

const char *balansun_source_logic_wire_for_id(SourceId id) {
  for (const auto &row : kWireTable) {
    if (row.id == id) return row.wire;
  }
  return "Unknown";
}

bool balansun_source_logic_meter_uart_pins_active(SourceId id) {
  return id == SourceId::JsyMk194 || id == SourceId::JsyMk333 || id == SourceId::Linky;
}

bool balansun_source_logic_meter_analog_pins_active(SourceId id) { return id == SourceId::Analog; }

bool balansun_source_logic_pin_field_allowed(const char *key, SourceId eff) {
  if (!key) return true;
  if (strcmp(key, "pin_triac_dim") == 0 || strcmp(key, "pin_zero_cross") == 0 ||
      strcmp(key, "pin_temp") == 0) {
    return true;
  }
  if (strcmp(key, "pin_uart_rx") == 0 || strcmp(key, "pin_uart_tx") == 0) {
    return balansun_source_logic_meter_uart_pins_active(eff);
  }
  if (strcmp(key, "pin_analog0") == 0 || strcmp(key, "pin_analog1") == 0 ||
      strcmp(key, "pin_analog2") == 0) {
    return balansun_source_logic_meter_analog_pins_active(eff);
  }
  return true;
}

bool balansun_source_logic_cap_mqtt_triac_channel_block_for(SourceId id) {
  return id == SourceId::JsyMk194 || id == SourceId::JsyMk333 || id == SourceId::ShellyEm;
}

bool balansun_source_logic_second_channel_snapshot_visible(float voltage_v, int active_import_w,
                                                        int active_export_w, float current_a) {
  return voltage_v > 0.1f || active_import_w != 0 || active_export_w != 0 || current_a > 0.01f;
}

bool balansun_source_logic_cap_mqtt_linky_tariff_for(SourceId id, bool tempo_rte_enabled) {
  return id == SourceId::Linky || tempo_rte_enabled;
}

bool balansun_source_logic_cap_serial_adc_gpio_restrict_for(SourceId id) {
  return id == SourceId::Analog || id == SourceId::JsyMk333;
}

uint16_t balansun_source_logic_base_poll_period_ms(SourceId id, uint32_t jsy_mk333_serial_baud) {
  switch (id) {
    case SourceId::JsyMk194:
      return 400;
    case SourceId::JsyMk333:
      return (jsy_mk333_serial_baud == 19200u) ? 500 : 800;
    case SourceId::Analog:
      return 40;
    case SourceId::Linky:
      return 2;
    case SourceId::Enphase:
      return 600;
    case SourceId::ShellyEm:
    case SourceId::ShellyPro:
    case SourceId::SmartG:
    case SourceId::HomeW:
      return 300;
    case SourceId::BalansunPeer:
      return 800;
    case SourceId::Pmqtt:
    case SourceId::NotDef:
      return 600;
    default:
      return 1000;
  }
}
