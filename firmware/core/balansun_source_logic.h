#pragma once

/*
 * balansun_source_logic.h — Pure source registry/capabilities (host-testable, no Arduino I/O).
 * Maps wire strings (Analog, JsyMk194, Linky, BalansunPeer, …) to SourceId and capability flags.
 */

#include "balansun_source_types.h"

#include <cstddef>

size_t balansun_source_logic_registry_count();
const char *balansun_source_logic_wire_at(size_t i);
SourceId balansun_source_logic_parse_wire(const char *wire);
/** Passthrough wire label (canonical only; no legacy alias mapping). */
const char *balansun_source_logic_normalize_wire_cstr(const char *wire);
SourceId balansun_source_logic_effective_id(SourceId active, const char *source_data_wire);
const char *balansun_source_logic_wire_for_id(SourceId id);

/** UART meter GPIO (`pin_uart_rx` / `pin_uart_tx`) — JsyMk194, JsyMk333, Linky. */
bool balansun_source_logic_meter_uart_pins_active(SourceId id);
/** Analog meter GPIO (`pin_analog*`) — Analog only. */
bool balansun_source_logic_meter_analog_pins_active(SourceId id);
/** True when `key` is a `pin_*` config field allowed for the effective measurement source. */
bool balansun_source_logic_pin_field_allowed(const char *key, SourceId eff);

bool balansun_source_logic_cap_mqtt_triac_channel_block_for(SourceId id);
/** True when CH2 metering should appear on REST/MQTT state snapshot (any source). */
bool balansun_source_logic_second_channel_snapshot_visible(float voltage_v, int active_import_w,
                                                        int active_export_w, float current_a);
bool balansun_source_logic_cap_mqtt_linky_tariff_for(SourceId id, bool tempo_rte_enabled);
bool balansun_source_logic_cap_serial_adc_gpio_restrict_for(SourceId id);

/** Default base poll period (ms) before house-import backoff. JsyMk333: 500 @ 19200 baud, else 800. */
uint16_t balansun_source_logic_base_poll_period_ms(SourceId id, uint32_t jsy_mk333_serial_baud);
