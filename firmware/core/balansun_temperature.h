#pragma once

#include "balansun_temperature_logic.h"

#include <ArduinoJson.h>

#include <cstdint>

extern int tempGpio;
extern BalansunTempSlotConfig temperatureSlots[kBalansunTempMaxSensors];
extern BalansunTempSlotState g_temperature_slot_states[kBalansunTempMaxSensors];
extern int g_temperature_discovered_count;
extern bool g_temperature_bus_active;

void balansun_temperature_init_defaults();
void balansun_temperature_sync_legacy_label();
int balansun_temperature_effective_gpio();
bool balansun_temperature_bus_should_run();
void balansun_temperature_poll();
/** Drop cached 1-Wire bus (e.g. after hardware pin change). Next poll re-inits. */
void balansun_temperature_invalidate_bus();
/** Non-blocking 1-Wire service: finish conversions and idle poll (~5 s). Call from main loop. */
void balansun_temperature_service(unsigned long now_ms);
void balansun_temperature_append_telemetry_json(JsonObject root);
void balansun_temperature_append_config_json(JsonObject root);
bool balansun_temperature_apply_config_json(JsonObject root, bool has_slots, bool has_gpio,
                                        bool has_legacy_label, String &err);
/** Active enabled slots with a valid reading (for hw presence detail). */
int balansun_temperature_active_valid_count();

struct EepromExtensionFields;
void balansun_temperature_load_extension_fields(const EepromExtensionFields &fields);
void balansun_temperature_store_extension_fields(EepromExtensionFields &fields);
