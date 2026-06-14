#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_PMqtt
#include "pmqtt_bindings.h"

#include "balansun_api_ready_logic.h"
#include "balansun_globals.h"
#include "balansun_meter_json.h"
#include "balansun_pub.h"
#include "balansun_regulation_state.h"

#include <ArduinoJson.h>
#include "balansun_json.h"

namespace {

struct RuntimeMetricState {
  String metric;
  String topic;
  bool ok = false;
  float value = 0.0f;
  String last_error;
  unsigned long last_rx_ms = 0;
};

std::vector<PmqttBindingEntry> g_cachedBindings;
String g_cachedBindingsSource;
std::vector<RuntimeMetricState> g_metricStates;
std::vector<String> g_topicCacheTopics;
std::vector<String> g_topicCachePayloads;
std::vector<unsigned long> g_topicCacheAt;

String normalize_path(String path) {
  path.trim();
  if (path == "$") return "";
  if (path.startsWith("$.")) path.remove(0, 2);
  if (path.startsWith("$")) path.remove(0, 1);
  while (path.startsWith(".")) path.remove(0, 1);
  return path;
}

bool get_json_by_path(JsonVariantConst root, const String &inPath, JsonVariantConst &out) {
  const String path = normalize_path(inPath);
  if (path.length() == 0) {
    out = root;
    return true;
  }
  JsonVariantConst cur = root;
  int start = 0;
  while (start < path.length()) {
    const int dot = path.indexOf('.', start);
    const String key = (dot < 0) ? path.substring(start) : path.substring(start, dot);
    if (!cur.is<JsonObjectConst>()) return false;
    JsonObjectConst obj = cur.as<JsonObjectConst>();
    if (obj[key.c_str()].isNull()) return false;
    cur = obj[key];
    if (dot < 0) break;
    start = dot + 1;
  }
  out = cur;
  return true;
}

bool variant_to_float(JsonVariantConst v, float &out) {
  if (v.is<float>() || v.is<double>() || v.is<int>() || v.is<long>() || v.is<unsigned int>() ||
      v.is<unsigned long>()) {
    out = v.as<float>();
    return true;
  }
  if (v.is<const char *>()) {
    const char *s = v.as<const char *>();
    if (!s || !s[0]) return false;
    char *end = nullptr;
    const float f = strtof(s, &end);
    if (end == s) return false;
    out = f;
    return true;
  }
  return false;
}

RuntimeMetricState &metric_state_for(const String &metric, const String &topic) {
  for (auto &st : g_metricStates) {
    if (st.metric == metric && st.topic == topic) return st;
  }
  g_metricStates.push_back(RuntimeMetricState{});
  RuntimeMetricState &st = g_metricStates.back();
  st.metric = metric;
  st.topic = topic;
  return st;
}

void set_metric_ok(const PmqttBindingEntry &b, float value) {
  RuntimeMetricState &st = metric_state_for(b.metric, b.topic);
  st.ok = true;
  st.value = value;
  st.last_error = "";
  st.last_rx_ms = millis();
}

void set_metric_err(const PmqttBindingEntry &b, const String &err) {
  RuntimeMetricState &st = metric_state_for(b.metric, b.topic);
  st.ok = false;
  st.last_error = err;
  st.last_rx_ms = millis();
}

void split_signed_net_w(float pw) {
  PwMQTT_last = pw;
  if (pw >= 0.0f) {
    house_active_import_w = static_cast<int>(pw);
    house_active_export_w = 0;
  } else {
    house_active_import_w = 0;
    house_active_export_w = static_cast<int>(-pw);
  }
}

bool apply_metric_value(const PmqttBindingEntry &b, float value, String *err) {
  const String m = b.metric;
  if (m == "house.signed_net_w") {
    split_signed_net_w(value);
    return true;
  }
  if (m == "house.pf") {
    float pf = fabsf(value);
    if (pf < 0.01f) pf = 1.0f;
    if (pf > 1.0f) pf = 1.0f;
    if (house_active_import_w > 0) {
      house_apparent_import_va = static_cast<int>(house_active_import_w / pf);
      house_apparent_export_va = 0;
    } else if (house_active_export_w > 0) {
      house_apparent_export_va = static_cast<int>(house_active_export_w / pf);
      house_apparent_import_va = 0;
    }
    return true;
  }
  if (m == "house.active_import_w") {
    house_active_import_w = static_cast<int>(value);
    return true;
  }
  if (m == "house.active_export_w") {
    house_active_export_w = static_cast<int>(value);
    return true;
  }
  if (m == "house.apparent_import_va") {
    house_apparent_import_va = static_cast<int>(value);
    return true;
  }
  if (m == "house.apparent_export_va") {
    house_apparent_export_va = static_cast<int>(value);
    return true;
  }
  if (m == "house.energy_day_import_wh") {
    house_day_energy_import_wh = value;
    return true;
  }
  if (m == "house.energy_day_export_wh") {
    house_day_energy_export_wh = value;
    return true;
  }
  if (m == "house.energy_total_import_wh") {
    house_energy_import_wh = value;
    return true;
  }
  if (m == "house.energy_total_export_wh") {
    house_energy_export_wh = value;
    return true;
  }
  if (m == "second.active_import_w") {
    second_active_import_w = static_cast<int>(value);
    return true;
  }
  if (m == "second.active_export_w") {
    second_active_export_w = static_cast<int>(value);
    return true;
  }
  if (m == "second.apparent_import_va") {
    second_apparent_import_va = static_cast<int>(value);
    return true;
  }
  if (m == "second.apparent_export_va") {
    second_apparent_export_va = static_cast<int>(value);
    return true;
  }
  if (m == "second.energy_day_import_wh") {
    second_day_energy_import_wh = value;
    return true;
  }
  if (m == "second.energy_day_export_wh") {
    second_day_energy_export_wh = value;
    return true;
  }
  if (m == "second.energy_total_import_wh") {
    second_energy_import_wh = value;
    return true;
  }
  if (m == "second.energy_total_export_wh") {
    second_energy_export_wh = value;
    return true;
  }
  if (m == "raw_meter.voltage_house_v") {
    house_voltage_v = value;
    return true;
  }
  if (m == "raw_meter.current_house_a") {
    house_current_a = value;
    return true;
  }
  if (m == "raw_meter.pf_house") {
    house_power_factor = value;
    return true;
  }
  if (m == "raw_meter.voltage_second_v") {
    second_voltage_v = value;
    return true;
  }
  if (m == "raw_meter.current_second_a") {
    second_current_a = value;
    return true;
  }
  if (m == "raw_meter.pf_second") {
    second_power_factor = value;
    return true;
  }
  if (m == "raw_meter.freq_hz") {
    mains_frequency_hz = value;
    return true;
  }
  if (m == "triac.open_percent") {
    int open = static_cast<int>(value + (value >= 0.0f ? 0.5f : -0.5f));
    if (open < 0) open = 0;
    if (open > 100) open = 100;
    g_triac_delay_percent[0] = 100 - open;
    g_triac_delay_percent_f[0] = static_cast<float>(g_triac_delay_percent[0]);
    balansun_regulation_sync_triac_globals();
    return true;
  }
  if (err) *err = "unknown_metric";
  return false;
}

bool parse_and_cache(std::vector<PmqttBindingEntry> &out, String *err = nullptr) {
  if (g_cachedBindingsSource.length() > 0 && g_cachedBindingsSource == PmqttBindingsJson) {
    out = g_cachedBindings;
    return true;
  }
  out.clear();
  g_cachedBindings.clear();
  g_cachedBindingsSource = "";
  BalansunJsonDoc _balansunJsonPool1 = balansun_json_doc_alloc(24576);
  JsonDocument &doc = _balansunJsonPool1;
  DeserializationError de = deserializeJson(doc, PmqttBindingsJson.c_str(), PmqttBindingsJson.length());
  if (de) {
    if (err) *err = de.c_str();
    return false;
  }
  if (!doc.is<JsonArray>()) {
    if (err) *err = "pmqtt_bindings_not_array";
    return false;
  }
  JsonArray arr = doc.as<JsonArray>();
  for (JsonVariant v : arr) {
    if (!v.is<JsonObject>()) continue;
    JsonObject o = v.as<JsonObject>();
    PmqttBindingEntry b;
    b.metric = String(o["metric"] | "");
    b.topic = String(o["topic"] | "");
    b.format = String(o["format"] | "json");
    b.path = normalize_path(String(o["path"] | ""));
    b.enabled = !o["enabled"].isNull() ? o["enabled"].as<bool>() : true;
    b.metric.trim();
    b.topic.trim();
    b.format.trim();
    b.format.toLowerCase();
    if (b.metric.length() == 0 || b.topic.length() == 0 || !b.enabled) continue;
    out.push_back(b);
  }
  g_cachedBindings = out;
  g_cachedBindingsSource = PmqttBindingsJson;
  return true;
}

bool group_signed(const std::vector<PmqttBindingEntry> &bindings) {
  for (const auto &b : bindings) {
    if (b.enabled && b.metric == "house.signed_net_w") return true;
  }
  return false;
}

bool group_split(const std::vector<PmqttBindingEntry> &bindings) {
  bool imp = false;
  bool exp = false;
  for (const auto &b : bindings) {
    if (!b.enabled) continue;
    if (b.metric == "house.active_import_w") imp = true;
    if (b.metric == "house.active_export_w") exp = true;
  }
  return imp && exp;
}

bool group_snapshot(const std::vector<PmqttBindingEntry> &bindings) {
  for (const auto &b : bindings) {
    if (b.enabled && (b.metric == "house.snapshot")) return true;
  }
  return false;
}

void set_last_payload_for_topic(const String &topic, const String &payload) {
  for (size_t i = 0; i < g_topicCacheTopics.size(); i++) {
    if (g_topicCacheTopics[i] == topic) {
      g_topicCachePayloads[i] = payload;
      g_topicCacheAt[i] = millis();
      return;
    }
  }
  g_topicCacheTopics.push_back(topic);
  g_topicCachePayloads.push_back(payload);
  g_topicCacheAt.push_back(millis());
  if (g_topicCacheTopics.size() > 32) {
    g_topicCacheTopics.erase(g_topicCacheTopics.begin());
    g_topicCachePayloads.erase(g_topicCachePayloads.begin());
    g_topicCacheAt.erase(g_topicCacheAt.begin());
  }
}

bool get_last_payload_for_topic(const String &topic, String &payload, unsigned long &ageMs) {
  const unsigned long now = millis();
  for (size_t i = 0; i < g_topicCacheTopics.size(); i++) {
    if (g_topicCacheTopics[i] == topic) {
      payload = g_topicCachePayloads[i];
      ageMs = now - g_topicCacheAt[i];
      return true;
    }
  }
  return false;
}

}  // namespace

void pmqtt_bindings_invalidate_cache() {
  g_cachedBindingsSource = "";
  g_cachedBindings.clear();
}

bool pmqtt_bindings_array_has_required_power(JsonArrayConst arr) {
  bool imp = false;
  bool exp = false;
  for (JsonObjectConst o : arr) {
    if (!o["enabled"].isNull() && !o["enabled"].as<bool>()) continue;
    const char *metric = o["metric"] | "";
    if (strcmp(metric, "house.signed_net_w") == 0) return true;
    if (strcmp(metric, "house.snapshot") == 0) return true;
    if (strcmp(metric, "house.active_import_w") == 0) imp = true;
    if (strcmp(metric, "house.active_export_w") == 0) exp = true;
  }
  return imp && exp;
}

bool pmqtt_bindings_parse_config(std::vector<PmqttBindingEntry> &out, String *err) { return parse_and_cache(out, err); }

bool pmqtt_bindings_has_required_power(String *missingGroup) {
  std::vector<PmqttBindingEntry> bindings;
  if (!parse_and_cache(bindings, nullptr)) {
    if (missingGroup) *missingGroup = "group_parse";
    return false;
  }
  if (group_signed(bindings) || group_split(bindings) || group_snapshot(bindings)) return true;
  if (missingGroup) *missingGroup = "group_power";
  return false;
}

static bool metric_state_live(const String &metric) {
  for (const auto &st : g_metricStates) {
    if (st.metric == metric && st.ok && st.last_rx_ms > 0) return true;
  }
  return false;
}

bool pmqtt_bindings_config_has_enabled_entries() {
  std::vector<PmqttBindingEntry> bindings;
  if (!parse_and_cache(bindings, nullptr)) return false;
  for (const auto &b : bindings) {
    if (b.enabled) return true;
  }
  return false;
}

bool pmqtt_bindings_meter_ready() {
  std::vector<PmqttBindingEntry> bindings;
  if (!parse_and_cache(bindings, nullptr)) return false;
  if (group_signed(bindings)) {
    return balansun_pmqtt_bindings_ready_logic(PmqttBindingsPowerGroup::Signed,
                                            metric_state_live("house.signed_net_w"), false, false,
                                            false);
  }
  if (group_split(bindings)) {
    return balansun_pmqtt_bindings_ready_logic(
        PmqttBindingsPowerGroup::Split, false, metric_state_live("house.active_import_w"),
        metric_state_live("house.active_export_w"), false);
  }
  if (group_snapshot(bindings)) {
    return balansun_pmqtt_bindings_ready_logic(PmqttBindingsPowerGroup::Snapshot, false, false, false,
                                            metric_state_live("house.snapshot"));
  }
  return false;
}

static bool metric_is_day_energy(const String &metric) {
  return metric.endsWith(".energy_day_import_wh") || metric.endsWith(".energy_day_export_wh");
}

bool pmqtt_bindings_provides_day_energy() {
  std::vector<PmqttBindingEntry> bindings;
  if (!parse_and_cache(bindings, nullptr)) return false;
  for (const auto &b : bindings) {
    if (b.enabled && metric_is_day_energy(b.metric)) return true;
  }
  return false;
}

bool pmqtt_bindings_triac_open_percent_configured() {
  std::vector<PmqttBindingEntry> bindings;
  if (!parse_and_cache(bindings, nullptr)) return false;
  for (const auto &b : bindings) {
    if (b.enabled && b.metric == "triac.open_percent") return true;
  }
  return false;
}

bool pmqtt_bindings_triac_open_percent_live(unsigned long stale_ms) {
  if (!pmqtt_bindings_triac_open_percent_configured()) return false;
  const unsigned long now = millis();
  for (const auto &st : g_metricStates) {
    if (st.metric != "triac.open_percent" || !st.ok || st.last_rx_ms == 0) continue;
    if (now - st.last_rx_ms <= stale_ms) return true;
  }
  return false;
}

void pmqtt_bindings_collect_topics(std::vector<String> &outTopics) {
  outTopics.clear();
  std::vector<PmqttBindingEntry> bindings;
  if (!parse_and_cache(bindings, nullptr)) return;
  for (const auto &b : bindings) {
    bool exists = false;
    for (const auto &t : outTopics) {
      if (t == b.topic) {
        exists = true;
        break;
      }
    }
    if (!exists) outTopics.push_back(b.topic);
  }
}

bool pmqtt_bindings_apply_message(const String &topic, const String &payload, String *err) {
  std::vector<PmqttBindingEntry> bindings;
  if (!parse_and_cache(bindings, err)) return false;
  bool appliedAny = false;
  BalansunJsonDoc _balansunJsonPool2 = balansun_json_doc_alloc(4096);
  JsonDocument &doc = _balansunJsonPool2;
  bool jsonReady = false;
  for (const auto &b : bindings) {
    if (b.topic != topic || !b.enabled) continue;
    if (b.format == "plain") {
      String trimmed = payload;
      trimmed.trim();
      char *end = nullptr;
      const float f = strtof(trimmed.c_str(), &end);
      if (end == trimmed.c_str()) {
        set_metric_err(b, "plain_not_numeric");
        continue;
      }
      String metricErr;
      if (!apply_metric_value(b, f, &metricErr)) {
        set_metric_err(b, metricErr);
        continue;
      }
      set_metric_ok(b, f);
      appliedAny = true;
      continue;
    }
    if (!jsonReady) {
      DeserializationError de = deserializeJson(doc, payload);
      if (de) {
        set_metric_err(b, de.c_str());
        continue;
      }
      jsonReady = true;
    }
    if (b.format == "snapshot") {
      JsonVariantConst node = doc.as<JsonVariantConst>();
      JsonVariantConst selected;
      if (!get_json_by_path(node, b.path, selected)) {
        set_metric_err(b, "snapshot_path_not_found");
        continue;
      }
      if (!selected.is<JsonObjectConst>()) {
        set_metric_err(b, "snapshot_path_not_object");
        continue;
      }
      BalansunJsonDoc _balansunJsonPool3 = balansun_json_doc_alloc(4096);
  JsonDocument &wrapper = _balansunJsonPool3;
      JsonObject root = wrapper.to<JsonObject>();
      if (b.path.startsWith("second")) {
        root["second"] = selected.as<JsonObjectConst>();
      } else {
        root["house"] = selected.as<JsonObjectConst>();
      }
      String applyErr;
      if (!ApplyMeterSnapshotFromJson(root, &applyErr)) {
        set_metric_err(b, applyErr);
        continue;
      }
      set_metric_ok(b, static_cast<float>(house_active_import_w - house_active_export_w));
      appliedAny = true;
      continue;
    }
    JsonVariantConst node = doc.as<JsonVariantConst>();
    JsonVariantConst selected;
    if (!get_json_by_path(node, b.path, selected)) {
      set_metric_err(b, "json_path_not_found");
      continue;
    }
    float f = 0;
    if (!variant_to_float(selected, f)) {
      set_metric_err(b, "json_value_not_numeric");
      continue;
    }
    String metricErr;
    if (!apply_metric_value(b, f, &metricErr)) {
      set_metric_err(b, metricErr);
      continue;
    }
    set_metric_ok(b, f);
    appliedAny = true;
  }
  if (appliedAny) {
    LastPwMQTTMillis = millis();
    PwMQTT_last = static_cast<float>(house_active_import_w - house_active_export_w);
    if (pmqtt_bindings_meter_ready()) {
      meter_reading_valid = true;
    }
    BalansunPublishFromGlobals();
  } else if (err) {
    *err = "no_binding_applied";
  }
  return appliedAny;
}

void pmqtt_bindings_cache_payload(const String &topic, const String &payload) { set_last_payload_for_topic(topic, payload); }

void pmqtt_bindings_append_diagnostics(JsonObject root) {
  JsonObject pmqtt = root["pmqtt"].to<JsonObject>();
  JsonArray arr = pmqtt["bindings"].to<JsonArray>();
  const unsigned long now = millis();
  for (const auto &st : g_metricStates) {
    JsonObject b = arr.add<JsonObject>();
    b["metric"] = st.metric;
    b["topic"] = st.topic;
    b["ok"] = st.ok;
    b["value"] = st.value;
    b["last_error"] = st.last_error;
    b["last_rx_ms_ago"] = st.last_rx_ms == 0 ? -1 : static_cast<int>(now - st.last_rx_ms);
  }
}

bool pmqtt_bindings_preview(JsonArray inputBindings, JsonArray outResults, String *err) {
  for (JsonVariantConst v : inputBindings) {
    if (!v.is<JsonObjectConst>()) continue;
    JsonObjectConst in = v.as<JsonObjectConst>();
    const String metric = String(in["metric"] | "");
    const String topic = String(in["topic"] | "");
    const String format = String(in["format"] | "json");
    String path = String(in["path"] | "");
    path = normalize_path(path);
    JsonObject o = outResults.add<JsonObject>();
    o["metric"] = metric;
    o["topic"] = topic;
    o["format"] = format;
    if (path.length()) o["path"] = path;
    if (metric.length() == 0 || topic.length() == 0) {
      o["ok"] = false;
      o["error"] = "invalid_binding";
      continue;
    }
    String payload;
    unsigned long ageMs = 0;
    if (!get_last_payload_for_topic(topic, payload, ageMs)) {
      o["ok"] = false;
      o["error"] = "no_message_yet";
      continue;
    }
    o["age_ms"] = ageMs;
    String raw = payload;
    raw.trim();
    if (raw.length() > 120) raw = raw.substring(0, 120);
    o["raw_snippet"] = raw;
    PmqttBindingEntry b;
    b.metric = metric;
    b.topic = topic;
    b.format = format;
    b.path = path;
    b.enabled = true;
    BalansunJsonDoc _balansunJsonPool4 = balansun_json_doc_alloc(4096);
  JsonDocument &doc = _balansunJsonPool4;
    float value = 0;
    if (format == "plain") {
      char *end = nullptr;
      value = strtof(payload.c_str(), &end);
      if (end == payload.c_str()) {
        o["ok"] = false;
        o["error"] = "plain_not_numeric";
        continue;
      }
    } else if (format == "json") {
      if (deserializeJson(doc, payload)) {
        o["ok"] = false;
        o["error"] = "json_parse";
        continue;
      }
      JsonVariantConst selected;
      if (!get_json_by_path(doc.as<JsonVariantConst>(), path, selected)) {
        o["ok"] = false;
        o["error"] = "json_path_not_found";
        continue;
      }
      if (!variant_to_float(selected, value)) {
        o["ok"] = false;
        o["error"] = "json_value_not_numeric";
        continue;
      }
    } else if (format == "snapshot") {
      if (deserializeJson(doc, payload)) {
        o["ok"] = false;
        o["error"] = "json_parse";
        continue;
      }
      JsonVariantConst selected;
      if (!get_json_by_path(doc.as<JsonVariantConst>(), path, selected) || !selected.is<JsonObjectConst>()) {
        o["ok"] = false;
        o["error"] = "snapshot_path_not_object";
        continue;
      }
      o["ok"] = true;
      o["value"] = 0;
      o["display"] = "snapshot_ok";
      continue;
    } else {
      o["ok"] = false;
      o["error"] = "format_not_supported";
      continue;
    }
    o["ok"] = true;
    o["value"] = value;
    if (fabsf(value) >= 1000.0f) {
      o["display"] = String(value / 1000.0f, 2) + " kW";
    } else {
      o["display"] = String(value, 1) + " W";
    }
  }
  if (err) *err = "";
  return true;
}

#else /* !BALANSUN_ENABLE_SOURCE_PMqtt */

#include "pmqtt_bindings.h"

void pmqtt_bindings_invalidate_cache() {}
bool pmqtt_bindings_parse_config(std::vector<PmqttBindingEntry> &, String *) { return false; }
bool pmqtt_bindings_array_has_required_power(JsonArrayConst) { return false; }
bool pmqtt_bindings_has_required_power(String *) { return false; }
bool pmqtt_bindings_config_has_enabled_entries() { return false; }
bool pmqtt_bindings_meter_ready() { return false; }
bool pmqtt_bindings_provides_day_energy() { return false; }
bool pmqtt_bindings_triac_open_percent_configured() { return false; }
bool pmqtt_bindings_triac_open_percent_live(unsigned long) { return false; }
void pmqtt_bindings_collect_topics(std::vector<String> &) {}
bool pmqtt_bindings_apply_message(const String &, const String &, String *) { return false; }
void pmqtt_bindings_cache_payload(const String &, const String &) {}
void pmqtt_bindings_append_diagnostics(JsonObject) {}
bool pmqtt_bindings_preview(JsonArray, JsonArray, String *) { return false; }

#endif /* BALANSUN_ENABLE_SOURCE_PMqtt */
