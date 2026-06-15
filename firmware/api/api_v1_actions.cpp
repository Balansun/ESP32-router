/*
 * Auto-split from api_v1_routes.cpp — see api_v1_common.h
 */
#include "api_v1_common.h"
#include "balansun_api_caps.h"
#include "balansun_temperature.h"
#include "balansun_hw_profile.h"
#include "balansun_regulation_modes.h"
void handle_get_actions_live() {
  API_AUTH_GUARD();
  BalansunJsonDoc _balansunJsonPool1 = balansun_json_doc_alloc(2048);
  JsonDocument &doc = _balansunJsonPool1;
  balansun_temperature_set_primary_c_json(doc.as<JsonObject>());
  balansun_temperature_append_telemetry_json(doc.as<JsonObject>());
  doc["source"] = Source_data;
  doc["ext_peer_ip"] = ip32ToDotted(ext_peer_ip);
  doc["ext_peer_port"] = ext_peer_port;
  doc["ext_peer_path"] = ext_peer_path;
  int nb = 0;
  for (int i = 0; i < NbActions; i++) {
    if (action_regulation_enabled(load_channels[i].Actif)) nb++;
  }
  doc["active_actions_count"] = nb;
  JsonArray slots = doc["active_slots"].to<JsonArray>();
  api_action_append_live_state(slots);
  String out;
  serializeJson(doc, out);
  api_send_json(server,200, out);
}

void handle_get_actions_schema() {
  API_AUTH_GUARD();
  BalansunJsonDoc _balansunJsonPool2 = balansun_json_doc_alloc(1536);
  JsonDocument &doc = _balansunJsonPool2;
  api_action_append_schema(doc.to<JsonObject>());
  String out;
  serializeJson(doc, out);
  api_send_json(server,200, out);
}

void handle_get_actions_config() {
  API_AUTH_GUARD();
  BalansunJsonDoc _balansunJsonPool3 = balansun_json_doc_alloc(12288);
  JsonDocument &doc = _balansunJsonPool3;
  doc["schema_version"] = API_ACTION_SCHEMA_VERSION;
  doc["nb_actions"] = NbActions;
  JsonArray ar = doc["actions"].to<JsonArray>();
  api_action_append_config_array(ar);
  String out;
  serializeJson(doc, out);
  api_send_json(server,200, out);
}

void handle_put_actions_config() {
  API_AUTH_GUARD();
  API_CAP_GUARD(BalansunCap::SurplusRegulation);
  const BalansunHwCaps caps = balansun_hw_caps_now();
  if (api_reject_oversized_body(server, caps.put_body_max)) return;
  BalansunJsonDoc _balansunJsonPool4 = balansun_json_doc_alloc(caps.put_body_max);
  JsonDocument &doc = _balansunJsonPool4;
  if (!api_deserialize_json_body(server, doc, caps.put_body_max, false)) return;
  String err;
  if (!api_action_apply_collection_object(doc.as<JsonObject>(), err)) {
    api_error(server,400, "validation", err.c_str());
    return;
  }
  JsonDocument o;
  o["ok"] = true;
  String s;
  serializeJson(o, s);
  api_send_json(server,200, s);
}

void handle_patch_actions_config_batch() {
  API_AUTH_GUARD();
  API_CAP_GUARD(BalansunCap::SurplusRegulation);
  const BalansunHwCaps patchCaps = balansun_hw_caps_now();
  if (api_reject_oversized_body(server, patchCaps.put_body_max)) return;
  BalansunJsonDoc _balansunJsonPool5 = balansun_json_doc_alloc(patchCaps.put_body_max);
  JsonDocument &doc = _balansunJsonPool5;
  if (!api_deserialize_json_body(server, doc, patchCaps.put_body_max, true)) return;
  String err;
  if (!api_action_patch_collection_batch_object(doc.as<JsonObject>(), err)) {
    api_error(server,400, "validation", err.c_str());
    return;
  }
  JsonDocument o;
  o["ok"] = true;
  String s;
  serializeJson(o, s);
  api_send_json(server,200, s);
}

void handle_get_action_override(int idx) {
  API_AUTH_GUARD();
  BalansunJsonDoc _balansunJsonPool6 = balansun_json_doc_alloc(256);
  JsonDocument &doc = _balansunJsonPool6;
  api_append_action_override(doc.to<JsonObject>(), idx);
  String out;
  serializeJson(doc, out);
  api_send_json(server, 200, out);
}

void handle_post_action_override(int idx) {
  API_AUTH_GUARD();
  API_SAFETY_LOCKOUT_GUARD();
  API_CAP_GUARD(BalansunCap::SurplusRegulation);
  if (idx >= 1) API_CAP_GUARD(BalansunCap::MultiAction);
  JsonDocument doc;
  if (!api_deserialize_json_body(server, doc, 256, true)) return;
  const char *state = doc["state"] | "auto";
  int triacPercent = (int)(doc["triac_open_percent"] | 0);
  unsigned long durationSec = (unsigned long)(doc["duration_s"] | 0);
  String err;
  if (!ApiSetActionOverride(idx, state, triacPercent, durationSec, err)) {
    api_error(server, 400, "validation", err.c_str());
    return;
  }
  BalansunJsonDoc _balansunJsonPool7 = balansun_json_doc_alloc(256);
  JsonDocument &outDoc = _balansunJsonPool7;
  JsonObject o = outDoc.to<JsonObject>();
  o["ok"] = true;
  JsonObject ov = o["override"].to<JsonObject>();
  api_append_action_override(ov, idx);
  String out;
  serializeJson(outDoc, out);
  api_send_json(server, 200, out);
}

void handle_clear_action_override(int idx) {
  API_AUTH_GUARD();
  API_SAFETY_LOCKOUT_GUARD();
  API_CAP_GUARD(BalansunCap::SurplusRegulation);
  if (idx >= 1) API_CAP_GUARD(BalansunCap::MultiAction);
  String err;
  if (!ApiSetActionOverride(idx, "auto", 0, 0, err)) {
    api_error(server, 400, "validation", err.c_str());
    return;
  }
  api_send_json(server, 200, "{\"ok\":true,\"state\":\"auto\"}");
}

