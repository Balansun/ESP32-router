#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

struct PmqttBindingEntry {
  String metric;
  String topic;
  String format;  // plain | json | snapshot
  String path;
  bool enabled = true;
};

struct PmqttBindingPreview {
  String metric;
  String topic;
  bool ok = false;
  float value = 0.0f;
  String error;
  String raw_snippet;
  unsigned long age_ms = 0;
};

void pmqtt_bindings_invalidate_cache();
bool pmqtt_bindings_parse_config(std::vector<PmqttBindingEntry> &out, String *err = nullptr);
/** Validate power group on a config JSON array without round-tripping through PmqttBindingsJson. */
bool pmqtt_bindings_array_has_required_power(JsonArrayConst arr);
bool pmqtt_bindings_has_required_power(String *missingGroup = nullptr);
/** True when bindings JSON has at least one enabled entry (vs legacy topic-only Pmqtt). */
bool pmqtt_bindings_config_has_enabled_entries();
/** True when required power group metrics have been received (ok + last_rx_ms). */
bool pmqtt_bindings_meter_ready();
/** True when active bindings include house/second day energy metrics (broker provides EnergieJour_*). */
bool pmqtt_bindings_provides_day_energy();
/** True when an enabled binding maps external triac open percent (e.g. Solar Router OuvertureTriac). */
bool pmqtt_bindings_triac_open_percent_configured();
/** Configured binding received a fresh value within stale_ms (falls back to local regulation when false). */
bool pmqtt_bindings_triac_open_percent_live(unsigned long stale_ms = 15000);
void pmqtt_bindings_collect_topics(std::vector<String> &outTopics);
bool pmqtt_bindings_apply_message(const String &topic, const String &payload, String *err = nullptr);
void pmqtt_bindings_cache_payload(const String &topic, const String &payload);
void pmqtt_bindings_append_diagnostics(JsonObject root);
bool pmqtt_bindings_preview(JsonArray inputBindings, JsonArray outResults, String *err = nullptr);
