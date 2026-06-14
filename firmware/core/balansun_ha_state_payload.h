#pragma once

/*
 * balansun_ha_state_payload.h — MQTT HA state topic and REST telemetry/snapshot share one payload shape.
 */

#include <ArduinoJson.h>

/** Source health score 0–100 (same inputs as MQTT discovery). */
int balansun_compute_source_health_score(void);

/** Stale when health_score < 50 (matches MQTT source_stale). */
bool balansun_source_health_is_stale(int health_score);

/** Append measurement diagnostics parity fields (regulation_hunting, site_cap, source_health, …). */
void balansun_append_measurements_diagnostics(JsonObject diag);

/** Append keys identical to MQTT `{prefix}/{device}_state` JSON (capability-gated). */
void balansun_append_ha_state_payload(JsonObject doc);

/** Apply triac command string (AUTO or 0–100); same as MQTT triac/set. */
bool balansun_apply_triac_command(const char *msg, String &err);
