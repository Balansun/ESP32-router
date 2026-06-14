#pragma once

#include <ArduinoJson.h>

/** Poll RTE Tempo open data when enabled and network time are valid. Call from main loop. */
void tempo_rte_poll(void);

/** Sync Arduino globals from internal Tempo state (after EEPROM load). */
void tempo_rte_sync_globals_from_state(void);

/** Copy globals into internal state before EEPROM save. */
void tempo_rte_sync_state_from_globals(void);

/** Fill Tempo fields for GET /api/v1/tariff/tempo JSON. */
void tempo_rte_append_api_json(JsonObject doc);
