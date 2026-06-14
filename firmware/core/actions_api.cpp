#include "actions_api.h"
#include "Actions.h"
#include "actions_logic.h"
#include "balansun_channel_state_logic.h"
#include "balansun_globals.h"
#include "balansun_product_caps.h"
#include "balansun_psram.h"
#include "balansun_json.h"
#include "balansun_regulation_modes.h"
#include "balansun_wire_separators.h"
#include "triac_api_shim.h"
#include <IPAddress.h>
#include <strings.h>

extern Action load_channels[];
extern int NbActions;

extern int persistConfigToEeprom(void);
extern void balansun_init_action_gpios(void);

#ifndef kMaxRoutingActions
#define kMaxRoutingActions 20
#endif

namespace {

constexpr size_t kActionsEepromJsonScratchCap = 16384;
char *g_actionsEepromJsonScratch = nullptr;

}  // namespace

size_t balansun_actions_eeprom_json_scratch_cap() { return kActionsEepromJsonScratchCap; }

char *balansun_actions_eeprom_json_scratch() {
  if (!g_actionsEepromJsonScratch) {
    g_actionsEepromJsonScratch = static_cast<char *>(balansun_cache_malloc(kActionsEepromJsonScratchCap));
  }
  return g_actionsEepromJsonScratch;
}

size_t balansun_actions_serialize_eeprom_json(char *buf, size_t cap) {
  if (!buf || cap < 4) return 0;
  BalansunJsonDoc _balansunJsonPool1 = balansun_json_doc_alloc(12288);
  JsonDocument &doc = _balansunJsonPool1;
  JsonArray ar = doc["actions"].to<JsonArray>();
  api_action_append_config_array(ar);
  if (doc.overflowed()) return 0;
  if (measureJson(doc) >= cap) return 0;
  const size_t written = serializeJson(doc, buf, cap);
  return written;
}

static const char *kModeOff = "off";
static const char *kModeOn = "on";
static const char *kModePower = "power";

static byte modeStrToType(const char *m) {
  if (!m) return 0;
  if (strcasecmp(m, kModeOff) == 0) return 1;
  if (strcasecmp(m, kModeOn) == 0) return 2;
  if (strcasecmp(m, kModePower) == 0) return 3;
  return 0;
}

static const char *typeToModeStr(byte t) {
  if (t == 1) return kModeOff;
  if (t == 2) return kModeOn;
  if (t == 3) return kModePower;
  return "unknown";
}

static void appendPeriod(JsonArray periods, int idx, int p) {
  JsonObject po = periods.add<JsonObject>();
  po["mode"] = typeToModeStr(load_channels[idx].Type[p]);
  po["hour_end"] = load_channels[idx].period_end[p];
  po["power_min_w"] = load_channels[idx].power_min[p];
  po["power_max_w"] = load_channels[idx].power_max[p];
  po["temp_inf_c"] = load_channels[idx].temp_min[p];
  po["temp_sup_c"] = load_channels[idx].temp_max[p];
}

void api_action_append_one_config(JsonObject out, int index) {
  if (index < 0 || index >= NbActions) return;
  out["index"] = index;
  out["regulation_mode"] = load_channels[index].Actif;
  out["title"] = load_channels[index].title;
  if (index == 0) {
    out["kind"] = "triac";
    out["triac_sensitivity"] = load_channels[index].Ki;
    out["ki"] = load_channels[index].Ki;
    out["kp"] = load_channels[index].Kp;
    out["kd"] = load_channels[index].Kd;
    out["pid_enabled"] = load_channels[index].PID;
  } else if (load_channels[index].Host == "localhost") {
    out["kind"] = "local_gpio";
    out["host"] = "localhost";
    out["port"] = load_channels[index].Port;
    out["path_on"] = load_channels[index].path_on;
    out["path_off"] = load_channels[index].path_off;
  } else {
    out["kind"] = "remote_http";
    out["host"] = load_channels[index].Host;
    out["port"] = load_channels[index].Port;
    out["path_on"] = load_channels[index].path_on;
    out["path_off"] = load_channels[index].path_off;
  }
  out["repeat_sec"] = load_channels[index].Repet;
  out["tempo_sec"] = load_channels[index].Tempo;
  JsonArray per = out["periods"].to<JsonArray>();
  const int periodN = actions_logic_clamp_period_count(load_channels[index].periodCount);
  for (int p = 0; p < periodN; p++) {
    appendPeriod(per, index, p);
  }
}

void api_action_append_config_array(JsonArray actionsOut) {
  for (int i = 0; i < NbActions; i++) {
    JsonObject o = actionsOut.add<JsonObject>();
    api_action_append_one_config(o, i);
  }
}

bool api_action_slot_needs_export(int index) {
  if (index < 0 || index >= NbActions) return false;
  if (index == 0) {
    if (action_regulation_enabled(load_channels[0].Actif)) return true;
    if (load_channels[0].Ki != 1 || load_channels[0].Kp != 0 || load_channels[0].Kd != 0) return true;
    if (load_channels[0].PID) return true;
    return false;
  }
  if (action_regulation_enabled(load_channels[index].Actif)) return true;
  if (load_channels[index].title.length() > 0) return true;
  if (load_channels[index].Host.length() > 0 && load_channels[index].Host != "localhost") return true;
  if (load_channels[index].path_on.length() > 0 || load_channels[index].path_off.length() > 0) return true;
  return false;
}

void api_action_append_sparse_config_array(JsonArray actionsOut, int &exported_count) {
  exported_count = 0;
  for (int i = 0; i < NbActions; i++) {
    if (!api_action_slot_needs_export(i)) continue;
    JsonObject o = actionsOut.add<JsonObject>();
    api_action_append_one_config(o, i);
    exported_count++;
  }
}

void actions_logic_override_tick_all(unsigned long now_ms) {
  for (int i = 0; i < NbActions; i++) {
    Action &act = load_channels[i];
    if (actions_logic_override_expired(act.OverrideState, act.OverrideUntilMillis, now_ms)) {
      act.ClearOverride();
    }
  }
}

void api_action_append_live_state(JsonArray out) {
  const BalansunProductCaps caps = balansun_product_caps_compile_time();
  for (int i = 0; i < NbActions; i++) {
    Action &act = load_channels[i];
    if (!action_regulation_enabled(act.Actif)) continue;
    JsonObject o = out.add<JsonObject>();
    o["index"] = i;
    o["title"] = act.title;
    BalansunChannelStateInput cin{};
    cin.caps = caps;
    cin.action_index = i;
    cin.regulation_mode = act.Actif;
    cin.override_state = act.OverrideState;
    cin.cap_hit = (i >= 0 && i < kMaxRoutingActions) ? actionCapHit[i] : false;
    cin.heater_backoff = heaterLoadBackoffActive;
    cin.is_remote_host = (act.Host.length() > 0 && act.Host != "localhost");
    if (i == 0) {
      cin.schedule_type = act.current_triac_schedule_type(wall_clock_decihours, temperature);
      o["triac_open_percent"] = TriacGetOpenPercent();
    } else {
      cin.schedule_type = act.schedule_type_at(wall_clock_decihours);
      o["on"] = act.On;
    }
    const BalansunChannelStateResult cs = balansun_channel_state_compute(cin);
    o["channel_state"] = balansun_channel_state_wire(cs.state);
    o["channel_reason"] = cs.reason;
  }
}

void api_action_append_schema(JsonObject root) {
  root["schema_version"] = API_ACTION_SCHEMA_VERSION;
  root["max_actions"] = kMaxRoutingActions;
  root["index_0_role"] = "triac";
  root["period_modes"] = "off | on | power — maps to firmware types 1,2,3";
  root["time_scale"] = "hour_end uses same 0..2400 scale as wall_clock_decihours (minutes*10/6 style upper bound 2400)";
  JsonObject units = root["units"].to<JsonObject>();
  units["power"] = "W";
  units["temperature_band"] = "°C sentinel rules unchanged vs firmware temp_min/temp_max";
}

static void apply_regulationCoeffsFromJson(int index, JsonObject o) {
  if (index != 0) return;
  if (!o["ki"].isNull()) {
    load_channels[0].Ki = (byte)min(255, max(1, (int)o["ki"]));
  } else if (!o["triac_sensitivity"].isNull()) {
    load_channels[0].Ki = (byte)min(255, max(1, (int)o["triac_sensitivity"]));
  }
  if (!o["kp"].isNull()) load_channels[0].Kp = (byte)(int)o["kp"];
  if (!o["kd"].isNull()) load_channels[0].Kd = (byte)(int)o["kd"];
  if (!o["pid_enabled"].isNull()) load_channels[0].PID = o["pid_enabled"].as<bool>();
}

static uint8_t regulation_mode_from_json(JsonObject o) {
  if (!o["regulation_mode"].isNull()) {
    int m = (int)o["regulation_mode"];
    if (m >= kModeInactif && m <= kModeDemisinus) return static_cast<uint8_t>(m);
  }
  return kModeInactif;
}

static bool buildLineFromJson(JsonObject o, int index, String& line, String& err) {
  const uint8_t actif = regulation_mode_from_json(o);
  if (o["title"].isNull()) {
    err = "missing title";
    return false;
  }
  String title = o["title"].as<String>();
  String host;
  int port = 0;
  String ordreOn = "";
  String ordreOff = "";
  if (index == 0) {
    host = "";
    port = 80;
  } else {
    const char *kind = o["kind"] | "";
    if (strcasecmp(kind, "local_gpio") == 0) {
      host = "localhost";
    } else if (o["host"].isNull()) {
      err = "missing host";
      return false;
    } else {
      host = o["host"].as<String>();
    }
    port = (int)(o["port"] | 80);
    ordreOn = o["path_on"] | "";
    ordreOff = o["path_off"] | "";
  }
  int repet = (int)(o["repeat_sec"] | 0);
  int tempo = (int)(o["tempo_sec"] | 0);
  if (o["periods"].isNull() || !o["periods"].is<JsonArray>()) {
    err = "missing periods array";
    return false;
  }
  JsonArray pa = o["periods"].as<JsonArray>();
  if (pa.size() == 0 || pa.size() > 8) {
    err = "periods length 1..8";
    return false;
  }
  line = String((int)actif) + BALANSUN_WIRE_RS + title + BALANSUN_WIRE_RS + host + BALANSUN_WIRE_RS + String(port) + BALANSUN_WIRE_RS + ordreOn + BALANSUN_WIRE_RS + ordreOff + BALANSUN_WIRE_RS +
         String(repet) + BALANSUN_WIRE_RS + String(tempo) + BALANSUN_WIRE_RS + String((int)pa.size());
  for (size_t i = 0; i < pa.size(); i++) {
    JsonObject p = pa[i];
    if (p["mode"].isNull()) {
      err = "period missing mode";
      return false;
    }
    String ms = p["mode"].as<String>();
    byte ty = modeStrToType(ms.c_str());
    if (ty == 0) {
      err = "invalid period mode";
      return false;
    }
    int period_end_h = (int)(p["hour_end"] | 0);
    int power_min_w = (int)(p["power_min_w"] | 0);
    int power_max_w = (int)(p["power_max_w"] | 0);
    int temp_min_c = (int)(p["temp_inf_c"] | 0);
    int temp_max_c = (int)(p["temp_sup_c"] | 0);
    line += BALANSUN_WIRE_RS + String(ty) + BALANSUN_WIRE_RS + String(period_end_h) + BALANSUN_WIRE_RS + String(power_min_w) + BALANSUN_WIRE_RS + String(power_max_w) + BALANSUN_WIRE_RS +
           String(temp_min_c) + BALANSUN_WIRE_RS + String(temp_max_c);
  }
  return true;
}

static bool jsonToLine(JsonVariant v, int index, String& line, String& err) {
  if (!v.is<JsonObject>()) {
    err = "expected object";
    return false;
  }
  JsonObject o = v.as<JsonObject>();
  return buildLineFromJson(o, index, line, err);
}

void api_action_persist_and_init_gpio() {
  persistConfigToEeprom();
  balansun_init_action_gpios();
}

String balansun_actions_serialize_eeprom_json() {
  char *scratch = balansun_actions_eeprom_json_scratch();
  if (scratch) {
    const size_t n = balansun_actions_serialize_eeprom_json(scratch, kActionsEepromJsonScratchCap);
    if (n > 0) return String(scratch);
  }
  BalansunJsonDoc _balansunJsonPool2 = balansun_json_doc_alloc(12288);
  JsonDocument &doc = _balansunJsonPool2;
  JsonArray ar = doc["actions"].to<JsonArray>();
  api_action_append_config_array(ar);
  String out;
  serializeJson(doc, out);
  return out;
}

bool api_action_put_one(int index, const String& body, String& err) {
  if (index < 0 || index >= kMaxRoutingActions) {
    err = "index out of range";
    return false;
  }
  BalansunJsonDoc _balansunJsonPool3 = balansun_json_doc_alloc(6144);
  JsonDocument &doc = _balansunJsonPool3;
  DeserializationError e = deserializeJson(doc, body);
  if (e) {
    err = e.c_str();
    return false;
  }
  return api_action_put_one_object(index, doc.as<JsonObject>(), err);
}

bool api_action_put_one_object(int index, JsonObject obj, String& err) {
  if (index < 0 || index >= kMaxRoutingActions) {
    err = "index out of range";
    return false;
  }
  String line;
  if (!jsonToLine(obj, index, line, err)) return false;
  load_channels[index].parse_from_wire(line);
  apply_regulationCoeffsFromJson(index, obj);
  if (index + 1 > NbActions) NbActions = index + 1;
  api_action_persist_and_init_gpio();
  return true;
}

static bool apply_actions_collection_object(JsonObject doc, String &err) {
  if (doc["actions"].isNull() || !doc["actions"].is<JsonArray>()) {
    if (!doc["nb_actions"].isNull() && doc["nb_actions"].as<int>() == 0) {
      NbActions = 0;
      return true;
    }
    err = "missing actions array";
    return false;
  }
  JsonArray ar = doc["actions"].as<JsonArray>();
  const int nb_declared = doc["nb_actions"] | -1;
  if (ar.size() == 0) {
    if (nb_declared == 0) {
      NbActions = 0;
      return true;
    }
    err = "actions length invalid";
    return false;
  }
  if (ar.size() > (size_t)kMaxRoutingActions) {
    err = "actions length invalid";
    return false;
  }
  const int newCount = (int)ar.size();
  for (int i = 0; i < newCount; i++) {
    String line;
    if (!jsonToLine(ar[i], i, line, err)) return false;
    load_channels[i].parse_from_wire(line);
    apply_regulationCoeffsFromJson(i, ar[i].as<JsonObject>());
  }
  NbActions = newCount;
  return true;
}

static bool apply_actions_collection_json(const String &body, String &err) {
  BalansunJsonDoc _balansunJsonPool4 = balansun_json_doc_alloc(12288);
  JsonDocument &doc = _balansunJsonPool4;
  const DeserializationError e = deserializeJson(doc, body);
  if (e) {
    err = e.c_str();
    return false;
  }
  return apply_actions_collection_object(doc.as<JsonObject>(), err);
}

bool api_action_apply_collection_object(JsonObject actionsRoot, String &err, bool persistNow) {
  if (!apply_actions_collection_object(actionsRoot, err)) return false;
  if (persistNow) {
    api_action_persist_and_init_gpio();
  } else {
    balansun_init_action_gpios();
  }
  return true;
}

bool api_action_apply_collection_sparse(JsonObject actionsRoot, String &err, bool persistNow) {
  if (actionsRoot["actions"].isNull() || !actionsRoot["actions"].is<JsonArray>()) {
    if (!actionsRoot["nb_actions"].isNull() && actionsRoot["nb_actions"].as<int>() == 0) {
      return true;
    }
    err = "missing actions array";
    return false;
  }
  JsonArray ar = actionsRoot["actions"].as<JsonArray>();
  for (JsonVariant v : ar) {
    if (!v.is<JsonObject>()) {
      err = "expected object";
      return false;
    }
    JsonObject o = v.as<JsonObject>();
    const int index = o["index"] | -1;
    if (index < 0 || index >= kMaxRoutingActions) {
      err = "invalid action index";
      return false;
    }
    String line;
    if (!jsonToLine(o, index, line, err)) return false;
    load_channels[index].parse_from_wire(line);
    apply_regulationCoeffsFromJson(index, o);
    if (index + 1 > NbActions) NbActions = index + 1;
  }
  if (persistNow) {
    api_action_persist_and_init_gpio();
  } else {
    balansun_init_action_gpios();
  }
  return true;
}

bool api_action_put_collection(const String &body, String &err) {
  if (!apply_actions_collection_json(body, err)) return false;
  api_action_persist_and_init_gpio();
  return true;
}

bool balansun_actions_load_eeprom_json(const String &json, String &err) {
  err.clear();
  if (json.length() == 0) {
    return true;
  }
  BalansunJsonDoc _balansunJsonPoolLoad = balansun_json_doc_alloc(12288);
  JsonDocument &doc = _balansunJsonPoolLoad;
  const DeserializationError e = deserializeJson(doc, json);
  if (e) {
    err = e.c_str();
    return false;
  }
  if (doc.is<JsonArray>()) {
    BalansunJsonDoc _balansunJsonPoolWrap = balansun_json_doc_alloc(12288);
    JsonDocument &wrap = _balansunJsonPoolWrap;
    wrap["actions"] = doc.as<JsonArray>();
    wrap["nb_actions"] = doc.as<JsonArray>().size();
    return apply_actions_collection_object(wrap.as<JsonObject>(), err);
  }
  return apply_actions_collection_object(doc.as<JsonObject>(), err);
}

bool balansun_actions_sanitize_all(bool *any_changed) {
  bool changed = false;
  for (int i = 0; i < NbActions; i++) {
    if (load_channels[i].sanitizePeriodSchedule()) changed = true;
  }
  if (any_changed) *any_changed = changed;
  return changed;
}

bool api_action_patch_one(int index, const String& body, String& err) {
  if (index < 0 || index >= NbActions) {
    err = "index out of range for patch";
    return false;
  }
  BalansunJsonDoc _balansunJsonPool5 = balansun_json_doc_alloc(2048);
  JsonDocument &pat = _balansunJsonPool5;
  DeserializationError e = deserializeJson(pat, body);
  if (e) {
    err = e.c_str();
    return false;
  }
  return api_action_patch_one_object(index, pat.as<JsonObject>(), err);
}

bool api_action_patch_one_object(int index, JsonObject pobj, String& err) {
  if (index < 0 || index >= NbActions) {
    err = "index out of range for patch";
    return false;
  }
  BalansunJsonDoc _balansunJsonPool6 = balansun_json_doc_alloc(6144);
  JsonDocument &cur = _balansunJsonPool6;
  JsonObject base = cur.to<JsonObject>();
  api_action_append_one_config(base, index);
  for (JsonPair kv : pobj) {
    const char *k = kv.key().c_str();
    if (strcmp(k, "index") == 0) continue;
    if (strcmp(k, "periods") == 0) {
      base["periods"].set(kv.value());
    } else {
      base[kv.key()] = kv.value();
    }
  }
  String line;
  if (!jsonToLine(cur.as<JsonVariant>(), index, line, err)) return false;
  load_channels[index].parse_from_wire(line);
  apply_regulationCoeffsFromJson(index, pobj);
  api_action_persist_and_init_gpio();
  return true;
}

bool api_action_patch_collection_batch(const String& body, String& err) {
  BalansunJsonDoc _balansunJsonPool7 = balansun_json_doc_alloc(4096);
  JsonDocument &doc = _balansunJsonPool7;
  DeserializationError e = deserializeJson(doc, body);
  if (e) {
    err = e.c_str();
    return false;
  }
  return api_action_patch_collection_batch_object(doc.as<JsonObject>(), err);
}

bool api_action_patch_collection_batch_object(JsonObject root, String& err) {
  JsonArray upd = root["updates"].as<JsonArray>();
  if (upd.isNull()) {
    err = "missing updates";
    return false;
  }
  if (upd.size() == 0 || upd.size() > API_ACTION_PATCH_MAX_UPDATES) {
    err = "updates batch size";
    return false;
  }
  for (JsonObject u : upd) {
    if (u["index"].isNull()) {
      err = "update missing index";
      return false;
    }
    int idx = (int)u["index"];
    if (idx < 0 || idx >= NbActions) {
      err = "bad update index";
      return false;
    }
    JsonObject setObj = u["set"];
    if (setObj.isNull()) {
      err = "update missing set";
      return false;
    }
    if (!api_action_patch_one_object(idx, setObj, err)) return false;
  }
  return true;
}
