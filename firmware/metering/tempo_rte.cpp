/*
 * tempo_rte.cpp — EDF Tempo colors via api-couleur-tempo.fr (/api/jourTempo/today + /tomorrow).
 * Uses HTTP/1.0 on port 80 (avoids chunked bodies; documented for embedded clients).
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_LINKY
#include "tempo_rte.h"
#include "tempo_rte_logic.h"
#include "balansun_globals.h"
#include "balansun_lan_http_client.h"
#include "balansun_source.h"

#include <WiFi.h>
#include <time.h>

namespace {

constexpr const char kTempoApiHost[] = "www.api-couleur-tempo.fr";
constexpr const char kJourTempoTodayPath[] = "/api/jourTempo/today";
constexpr const char kJourTempoTomorrowPath[] = "/api/jourTempo/tomorrow";
constexpr uint32_t kTempoHttpTimeoutMs = 15000;

TempoRteState g_state;

void load_state_from_globals() {
  g_state.enabled = tempoRteEnabled;
  g_state.ltarf = std::string(LTARF.c_str());
  g_state.tomorrow_stge_hex = std::string(STGEt.c_str());
  g_state.today_color_label = std::string(rte_today.c_str());
  g_state.tomorrow_color_label = std::string(rte_tomorrow.c_str());
  g_state.last_poll_time_decihours = tempoRteLastPollDecihours;
  g_state.last_fetch_epoch = tempoRteLastFetchEpoch;
}

void store_state_to_globals() {
  tempoRteEnabled = g_state.enabled;
  LTARF = String(g_state.ltarf.c_str());
  STGEt = String(g_state.tomorrow_stge_hex.c_str());
  rte_today = String(g_state.today_color_label.c_str());
  rte_tomorrow = String(g_state.tomorrow_color_label.c_str());
  tempoRteLastPollDecihours = g_state.last_poll_time_decihours;
  tempoRteLastFetchEpoch = g_state.last_fetch_epoch;
  LTARFbin = tempo_rte_logic_ltarf_bin(g_state.ltarf);
}

bool network_allows_rte_fetch() {
  if (WiFi.getMode() != WIFI_STA) return false;
  return WiFi.status() == WL_CONNECTED;
}

bool active_source_is_linky() { return balansun_active_source_get() == SourceId::Linky; }

bool active_source_is_ext() { return balansun_active_source_get() == SourceId::BalansunPeer; }

bool fetch_jour_tempo_path(const char *path, std::string &json_out) {
  json_out.clear();
  String body;
  if (!balansun_lan_http_get(kTempoApiHost, 80, path, body, kTempoHttpTimeoutMs)) {
    Serial.print(F("Tempo API: HTTP GET failed "));
    Serial.println(path);
    return false;
  }
  const std::string wire(body.c_str());
  if (!tempo_rte_logic_body_looks_like_json(wire)) {
    Serial.print(F("Tempo API: not JSON "));
    Serial.println(path);
    return false;
  }
  json_out = wire;
  return true;
}

bool fetch_jour_tempo(std::string &today_json_out, std::string &tomorrow_json_out) {
  today_json_out.clear();
  tomorrow_json_out.clear();
  bool any = false;
  if (fetch_jour_tempo_path(kJourTempoTodayPath, today_json_out)) any = true;
  if (fetch_jour_tempo_path(kJourTempoTomorrowPath, tomorrow_json_out)) any = true;
  return any;
}

}  // namespace

void tempo_rte_sync_globals_from_state() { store_state_to_globals(); }

void tempo_rte_sync_state_from_globals() { load_state_from_globals(); }

void tempo_rte_append_api_json(JsonObject doc) {
  load_state_from_globals();
  doc["enabled"] = g_state.enabled;
  doc["ltarf"] = g_state.ltarf.c_str();
  doc["tomorrow_stge"] = g_state.tomorrow_stge_hex.c_str();
  doc["today_color"] = g_state.today_color_label.c_str();
  doc["tomorrow_color"] = g_state.tomorrow_color_label.c_str();
  doc["tariff_code"] = tempo_rte_logic_tariff_code(g_state.ltarf);
  doc["ltarf_bin"] = tempo_rte_logic_ltarf_bin(g_state.ltarf);
  const uint32_t now = static_cast<uint32_t>(time(nullptr));
  const bool stale =
      g_state.enabled && g_state.last_fetch_epoch > 0 && now > g_state.last_fetch_epoch + 86400u;
  doc["stale"] = stale;
  doc["last_fetch_epoch"] = g_state.last_fetch_epoch;
}

void tempo_rte_poll(void) {
  load_state_from_globals();
  const int half_hour_slot = tempo_rte_logic_half_hour_slot(wall_clock_decihours);
  tempo_rte_logic_reset_tomorrow_stge_at_1030(half_hour_slot, g_state.tomorrow_stge_hex);
  tempo_rte_logic_apply_6h_rollover(half_hour_slot, g_state);

  if (!time_sync_valid) {
    store_state_to_globals();
    return;
  }

  if (!tempo_rte_logic_should_poll(g_state, wall_clock_decihours, time_sync_valid, network_allows_rte_fetch())) {
    if (!g_state.enabled && !active_source_is_linky() && !active_source_is_ext()) {
      g_state.ltarf.clear();
      g_state.tomorrow_stge_hex = kStgeTomorrowUnset;
    }
    store_state_to_globals();
    return;
  }

  std::string today_json;
  std::string tomorrow_json;
  if (!fetch_jour_tempo(today_json, tomorrow_json)) {
    store_state_to_globals();
    return;
  }

  if (tempo_rte_logic_apply_jour_tempo_responses(today_json, tomorrow_json, g_state)) {
    g_state.last_poll_time_decihours = wall_clock_decihours;
    g_state.last_fetch_epoch = static_cast<uint32_t>(time(nullptr));
    Serial.print(F("Tempo API: "));
    Serial.print(g_state.ltarf.c_str());
    Serial.print(F(" | tomorrow "));
    Serial.println(g_state.tomorrow_color_label.c_str());
  } else {
    Serial.println(F("Tempo API: no valid color in response"));
  }
  store_state_to_globals();
}

#else /* !BALANSUN_ENABLE_SOURCE_LINKY */

#include "tempo_rte.h"

void tempo_rte_poll(void) {}
void tempo_rte_sync_globals_from_state(void) {}
void tempo_rte_sync_state_from_globals(void) {}
void tempo_rte_append_api_json(JsonObject) {}

#endif /* BALANSUN_ENABLE_SOURCE_LINKY */
