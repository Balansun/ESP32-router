#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

#define API_ACTION_SCHEMA_VERSION 4
#define API_ACTION_PATCH_MAX_UPDATES 3

void api_action_append_schema(JsonObject root);
void api_action_append_live_state(JsonArray out);
void actions_logic_override_tick_all(unsigned long now_ms);
void api_action_append_config_array(JsonArray actionsOut);
void api_action_append_sparse_config_array(JsonArray actionsOut, int &exported_count);
void api_action_append_one_config(JsonObject out, int index);
bool api_action_slot_needs_export(int index);
bool api_action_apply_collection_sparse(JsonObject actionsRoot, String &err, bool persistNow = false);

bool api_action_put_collection(const String& body, String& err);
/** Apply actions config object (`schema_version`, `nb_actions`, `actions`) without extra parse buffer. */
bool api_action_apply_collection_object(JsonObject actionsRoot, String &err, bool persistNow = true);
bool api_action_put_one(int index, const String& body, String& err);
bool api_action_put_one_object(int index, JsonObject obj, String& err);
bool api_action_patch_one(int index, const String& body, String& err);
bool api_action_patch_one_object(int index, JsonObject patch, String& err);
bool api_action_patch_collection_batch(const String& body, String& err);
bool api_action_patch_collection_batch_object(JsonObject root, String& err);

void api_action_persist_and_init_gpio();

/** EEPROM extension tail: full actions config as JSON (schema 4). */
size_t balansun_actions_eeprom_json_scratch_cap();
char *balansun_actions_eeprom_json_scratch();
size_t balansun_actions_serialize_eeprom_json(char *buf, size_t cap);
String balansun_actions_serialize_eeprom_json();
bool balansun_actions_load_eeprom_json(const String &json, String &err);
/** Repair corrupt periodCount/schedule in RAM; returns true if any channel was reset. */
bool balansun_actions_sanitize_all(bool *any_changed);
