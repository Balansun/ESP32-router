#pragma once

/*
 * balansun_source.h — Metering source registry, dispatch, diagnostics JSON, capability matrix.
 * Active source: global Source string; Ext uses Source_data for remote meter type.
 * See: /en/project-overview/ § Metering source; /en/hardware-pinout/ §17.
 */

#include <Arduino.h>
#include <ArduinoJson.h>

#include "balansun_source_health_logic.h"
#include "balansun_source_types.h"

/** Re-parse global `Source` into the active id (call after EEPROM load, REST, MQTT source/set, etc.). */
void balansun_active_source_refresh_from_global_string();

SourceId balansun_active_source_get();

/**
 * Meter profile for payloads and UI: local `Source`, except when `Source == "Ext"` where the
 * remote meter type is carried in `Source_data`.
 */
SourceId balansun_effective_meter_id();

/** Number of entries in the built-in source registry (wire labels / dispatch table). */
size_t balansun_source_registry_count();

/** Wire string at index `i` (0 .. count-1); registry order matches HA MQTT source select. */
const char *balansun_source_wire_at(size_t i);

/** Append `wire` string for `id`, or empty string if unknown. */
const char *balansun_source_wire_for_id(SourceId id);

/** Fill `doc` with meter-specific nested objects (and Pmqtt/Ext transport blocks). */
void balansun_sources_diagnostics_append_meter_payload(JsonObject doc, int linky_tail_max);

/** Nested `diagnostics` object: poll timing, meter id, Ext errors, mains frequency hints. */
void balansun_sources_diagnostics_append_summary(JsonObject doc);

/** JSON `panel` wire label for the brute-data page (same semantics as `balansun_effective_meter_id`). */
void balansun_sources_brute_panel_json(String &out);

/** One-shot hardware setup for the current `Source` (Serial, etc.) and `Source_data` rule. */
void balansun_source_apply_hardware_setup();

/**
 * Run one metering poll for the active source and update `poll_period_ms` / optional `last_metering_task_ms`.
 * @param pollBackoffMs same as `long(house_active_import_w / 10)` for HTTP-style sources.
 */
void balansun_source_run_poll_cycle(unsigned long pollBackoffMs);

bool balansun_source_wire_supported(const String &wire);

/** Build health score inputs from live metering globals (same rules as diagnostics JSON). */
void balansun_source_health_input_now(SourceHealthScoreInput &out);

/** Current source health score 0–100. */
int balansun_source_health_score_now(void);

/** True when health score is below 50 (matches MQTT `source_stale`). */
bool balansun_source_is_stale_now(void);

// --- Capability predicates (keep MQTT / REST / GPIO matrix in one place) ---

bool balansun_cap_mqtt_triac_channel_block_for(SourceId id);
bool balansun_cap_mqtt_triac_channel_block();

bool balansun_cap_mqtt_linky_tariff_for(SourceId id);
bool balansun_cap_mqtt_linky_tariff();

/** Serial/Analog-style GPIO restrictions (see IsRestrictedGpioWrite). */
bool balansun_cap_serial_adc_gpio_restrict_for(SourceId id);
bool balansun_cap_serial_adc_gpio_restrict();

