/*
 * balansun_source.cpp — Poll dispatch to metering/*.cpp, HA wire labels, JSON diagnostics.
 */
#include "balansun_source.h"
#include "balansun_meter_driver.h"
#include "balansun_meter_sources_enable.h"
#include "balansun_source_logic.h"
#include "balansun_source_health_logic.h"
#include "balansun_diag.h"
#include "balansun_forward.h"
#include "balansun_globals.h"
#include "balansun_mains_profile.h"
#include "api_util.h"
#include <ArduinoJson.h>

#if BALANSUN_ENABLE_SOURCE_ANALOG
extern void uxi_probe_setup(void);
extern void uxi_probe_poll(void);
#endif
#if BALANSUN_ENABLE_SOURCE_JSY_MK194
extern void jsy_mk194t_setup(void);
extern void jsy_mk194t_poll(void);
#endif
#if BALANSUN_ENABLE_SOURCE_JSY_MK333
extern void jsy_mk333_setup(void);
extern void jsy_mk333_poll(void);
#endif
#if BALANSUN_ENABLE_SOURCE_LINKY
extern void linky_meter_setup(void);
extern void linky_meter_poll(void);
#endif
#if BALANSUN_ENABLE_SOURCE_ENPHASE
extern void enphase_envoy_setup(void);
extern void enphase_envoy_poll(void);
#endif
#if BALANSUN_ENABLE_SOURCE_SHELLY_EM
extern void shelly_em_poll(void);
#endif
#if BALANSUN_ENABLE_SOURCE_SHELLY_PRO
extern void shelly_pro_em_poll(void);
#endif
#if BALANSUN_ENABLE_SOURCE_SMARTG
extern void smart_gateway_poll(void);
#endif
#if BALANSUN_ENABLE_SOURCE_HOMEW
extern void homewizard_poll(void);
#endif
#if BALANSUN_ENABLE_SOURCE_NOTDEF
extern void bench_sim_poll(void);
#endif
#if BALANSUN_ENABLE_SOURCE_EXT
extern void external_peer_poll(void);
#endif

static void poll_nop(void) {}

enum : uint8_t {
  RSF_NONE = 0,
  RSF_POLL_BACKOFF = 1,
  RSF_TOUCH_LAST_MS = 2,
};

static SourceId g_active_source = SourceId::Unknown;

static const BalansunMeterDriver kRegistry[] = {
#if BALANSUN_ENABLE_SOURCE_JSY_MK194
    BALANSUN_METER_ROW(SourceId::JsyMk194, "JsyMk194", jsy_mk194t_setup, jsy_mk194t_poll, 400, RSF_NONE)
#endif
#if BALANSUN_ENABLE_SOURCE_JSY_MK333
    BALANSUN_METER_ROW(SourceId::JsyMk333, "JsyMk333", jsy_mk333_setup, jsy_mk333_poll, 800, RSF_NONE)
#endif
#if BALANSUN_ENABLE_SOURCE_ANALOG
    BALANSUN_METER_ROW(SourceId::Analog, "Analog", uxi_probe_setup, uxi_probe_poll, 40, RSF_NONE)
#endif
#if BALANSUN_ENABLE_SOURCE_LINKY
    BALANSUN_METER_ROW(SourceId::Linky, "Linky", linky_meter_setup, linky_meter_poll, 2, RSF_NONE)
#endif
#if BALANSUN_ENABLE_SOURCE_ENPHASE
    BALANSUN_METER_ROW(SourceId::Enphase, "Enphase", enphase_envoy_setup, enphase_envoy_poll, 600,
                    (uint8_t)(RSF_POLL_BACKOFF | RSF_TOUCH_LAST_MS))
#endif
#if BALANSUN_ENABLE_SOURCE_SHELLY_EM
    BALANSUN_METER_ROW(SourceId::ShellyEm, "ShellyEm", nullptr, shelly_em_poll, 300,
                    (uint8_t)(RSF_POLL_BACKOFF | RSF_TOUCH_LAST_MS))
#endif
#if BALANSUN_ENABLE_SOURCE_SHELLY_PRO
    BALANSUN_METER_ROW(SourceId::ShellyPro, "ShellyPro", nullptr, shelly_pro_em_poll, 300,
                    (uint8_t)(RSF_POLL_BACKOFF | RSF_TOUCH_LAST_MS))
#endif
#if BALANSUN_ENABLE_SOURCE_SMARTG
    BALANSUN_METER_ROW(SourceId::SmartG, "SmartG", nullptr, smart_gateway_poll, 300,
                    (uint8_t)(RSF_POLL_BACKOFF | RSF_TOUCH_LAST_MS))
#endif
#if BALANSUN_ENABLE_SOURCE_HOMEW
    BALANSUN_METER_ROW(SourceId::HomeW, "HomeW", nullptr, homewizard_poll, 300,
                    (uint8_t)(RSF_POLL_BACKOFF | RSF_TOUCH_LAST_MS))
#endif
#if BALANSUN_ENABLE_SOURCE_PMqtt
    BALANSUN_METER_ROW(SourceId::Pmqtt, "Pmqtt", nullptr, poll_nop, 600, RSF_TOUCH_LAST_MS)
#endif
#if BALANSUN_ENABLE_SOURCE_NOTDEF
    BALANSUN_METER_ROW(SourceId::NotDef, "NotDef", nullptr, bench_sim_poll, 600, RSF_NONE)
#endif
#if BALANSUN_ENABLE_SOURCE_EXT
    BALANSUN_METER_ROW(SourceId::BalansunPeer, "BalansunPeer", nullptr, external_peer_poll, 800,
                    (uint8_t)(RSF_POLL_BACKOFF | RSF_TOUCH_LAST_MS))
#endif
};

static const BalansunMeterDriver *row_for_id(SourceId id) {
  for (size_t i = 0; i < sizeof(kRegistry) / sizeof(kRegistry[0]); i++) {
    if (kRegistry[i].id == id) {
      return &kRegistry[i];
    }
  }
  return nullptr;
}

static SourceId parse_wire(const String &w) { return balansun_source_logic_parse_wire(w.c_str()); }

void balansun_active_source_refresh_from_global_string() {
  g_active_source = parse_wire(String(Source.c_str()));
}

SourceId balansun_active_source_get() { return g_active_source; }

SourceId balansun_effective_meter_id() {
  return balansun_source_logic_effective_id(g_active_source, Source_data.c_str());
}

size_t balansun_source_registry_count() { return sizeof(kRegistry) / sizeof(kRegistry[0]); }

const char *balansun_source_wire_at(size_t i) {
  if (i >= balansun_source_registry_count()) return "";
  return kRegistry[i].wire;
}

const char *balansun_source_wire_for_id(SourceId id) {
  const BalansunMeterDriver *row = row_for_id(id);
  return row ? row->wire : "";
}

bool balansun_source_wire_supported(const String &wire) { return parse_wire(wire) != SourceId::Unknown; }

void balansun_source_apply_hardware_setup() {
  balansun_active_source_refresh_from_global_string();
  const BalansunMeterDriver *row = row_for_id(g_active_source);
  if (row && row->setup_fn) {
    row->setup_fn();
  }
#if BALANSUN_ENABLE_SOURCE_JSY_MK333
  if (g_active_source == SourceId::JsyMk333) {
    delay(100);
    jsy_mk333_send_request();
    poll_period_ms = balansun_source_logic_base_poll_period_ms(SourceId::JsyMk333, JsyMk333SerialBaud);
  }
#endif
  if (g_active_source != SourceId::BalansunPeer) {
    Source_data = Source;
  }
}

void balansun_source_run_poll_cycle(unsigned long pollBackoffMs) {
  const BalansunMeterDriver *row = row_for_id(g_active_source);
  if (!row) {
    poll_period_ms = 1000;
    return;
  }
  if (row->poll_fn) {
    row->poll_fn();
  }
  if (row->flags & RSF_TOUCH_LAST_MS) {
    last_metering_task_ms = millis();
  }
  uint32_t p = balansun_source_logic_base_poll_period_ms(g_active_source, JsyMk333SerialBaud);
  if (row->flags & RSF_POLL_BACKOFF) {
    p += (uint32_t)pollBackoffMs;
  }
  poll_period_ms = p;
  if (mains_frequency_hz >= 45.0f && mains_frequency_hz <= 65.0f) {
    balansun_mains_on_meter_frequency(mains_frequency_hz);
  }
  bool poll_ok = true;
  if (g_active_source == SourceId::BalansunPeer) {
    poll_ok = ext_peer_last_poll_ok;
  } else {
    poll_ok = time_sync_valid;
  }
  balansun_diag_on_source_poll_result(poll_ok);
}

namespace {

BalansunSourceHealthPollState balansun_source_health_poll_state_now(void) {
  BalansunSourceHealthPollState st{};
  st.active_source = balansun_active_source_get();
  st.ext_peer_last_poll_ok = ext_peer_last_poll_ok;
  st.time_sync_valid = time_sync_valid;
  if (last_metering_task_ms > 0) {
    st.last_poll_ms_ago = static_cast<int>((millis() - last_metering_task_ms) & 0x7FFFFFFF);
  }
  st.poll_period_ms = poll_period_ms;
  st.error_streak = g_source_error_streak;
  return st;
}

}  // namespace

void balansun_source_health_input_now(SourceHealthScoreInput &out) {
  balansun_source_health_input_from_poll_state(balansun_source_health_poll_state_now(), out);
}

int balansun_source_health_score_now(void) {
  return balansun_source_health_score_from_poll_state(balansun_source_health_poll_state_now());
}

bool balansun_source_is_stale_now(void) {
  return balansun_source_is_stale_from_poll_state(balansun_source_health_poll_state_now());
}

void balansun_sources_diagnostics_append_summary(JsonObject root) {
  JsonObject diag = root["diagnostics"].to<JsonObject>();
  const SourceId active = balansun_active_source_get();
  const SourceId eff = balansun_effective_meter_id();
  diag["active_source"] = balansun_source_wire_for_id(active);
  diag["meter"] = balansun_source_wire_for_id(eff);
  if (active == SourceId::BalansunPeer && Source_data.length() > 0) {
    diag["source_data"] = Source_data;
  }
  diag["poll_period_ms"] = poll_period_ms;
  if (last_metering_task_ms > 0) {
    diag["last_poll_ms_ago"] = (int)((millis() - last_metering_task_ms) & 0x7FFFFFFF);
  } else {
    diag["last_poll_ms_ago"] = -1;
  }
  if (active == SourceId::BalansunPeer) {
    diag["last_poll_ok"] = ext_peer_last_poll_ok;
    diag["last_error"] = ext_peer_last_poll_err;
    diag["protocol_used"] = ext_peer_last_poll_protocol.length() ? ext_peer_last_poll_protocol : "json";
    diag["ext_protocol"] = ext_peer_protocol.length() ? ext_peer_protocol : "json";
    if (ext_peer_last_poll_ms > 0) {
      diag["transport_last_poll_ms_ago"] =
          (int)((millis() - ext_peer_last_poll_ms) & 0x7FFFFFFF);
    } else {
      diag["transport_last_poll_ms_ago"] = -1;
    }
  } else {
    diag["last_poll_ok"] = time_sync_valid;
    diag["last_error"] = "";
  }
  diag["mains_frequency_hz"] = balansun_mains_effective_frequency_hz();
  diag["mains_frequency_source"] = balansun_mains_frequency_source_string();
  const char *warn = balansun_mains_frequency_warning_string();
  if (warn && warn[0]) {
    diag["mains_frequency_warning"] = warn;
  }
  SourceHealthScoreInput hin;
  balansun_source_health_input_now(hin);
  const SourceHealthScoreResult score = balansun_source_health_logic_compute(hin);
  diag["health_score"] = score.health_score;
  JsonObject factors = diag["health_score_factors"].to<JsonObject>();
  factors["freshness"] = score.freshness_pts;
  factors["poll_ok"] = score.poll_ok_pts;
  factors["streak"] = score.streak_pts;
  if (balansun_diag_uxi_adc_clipping_active()) {
    diag["adc_clipping"] = true;
  }
  if (g_regulation_hunting_active) {
    diag["regulation_hunting"] = true;
  }
}

void balansun_sources_diagnostics_append_meter_payload(JsonObject doc, int linky_tail_max) {
  const SourceId eff = balansun_effective_meter_id();

  if (eff == SourceId::Analog || eff == SourceId::JsyMk194) {
    JsonObject wf = doc["uxi_waveform"].to<JsonObject>();
    JsonArray vArr = wf["volt_m"].to<JsonArray>();
    JsonArray aArr = wf["amp_m"].to<JsonArray>();
    int i0 = 0;
    for (int i = 0; i < 100; i++) {
      int i1 = (i + 1) % 100;
      if (voltM[i] <= 0 && voltM[i1] > 0) {
        i0 = i1;
        break;
      }
    }
    for (int i = 0; i < 100; i++) {
      int j = (i + i0) % 100;
      vArr.add(voltM[j]);
      aArr.add(ampM[j]);
    }
  }

  if (eff == SourceId::Linky) {
    JsonObject ly = doc["linky"].to<JsonObject>();
    ly["ltarf"] = LTARF;
    ly["idx_raw"] = IdxDataRawLinky;
    int tailMax = linky_tail_max;
    if (tailMax <= 0 || tailMax > 2048) {
      tailMax = 768;
    }
    int idx = IdxDataRawLinky;
    int start = (idx - tailMax + 10000) % 10000;
    String tail;
    tail.reserve((unsigned)tailMax);
    int k = start;
    for (int i = 0; i < tailMax; i++) {
      char c = DataRawLinky[k];
      if (c == 0) {
        c = ' ';
      }
      if ((unsigned char)c < 32) {
        c = '.';
      }
      tail += c;
      k = (k + 1) % 10000;
    }
    ly["tail"] = tail;
    ly["tail_len"] = tailMax;
    ly["linky_eait_from_tic"] = LinkyEaitFromTic;
    ly["linky_sinsti_seen"] = LinkySinstiSeen;
    ly["cacsi_no_export"] =
        (!LinkySinstiSeen && !LinkyEaitFromTic && EASTvalid);
  }

  if (eff == SourceId::Enphase) {
    JsonObject ep = doc["enphase"].to<JsonObject>();
    ep["user"] = EnphaseUser;
    ep["serial"] = meter_channel;
    ep["has_user"] = EnphaseUser.length() > 0;
    ep["has_session"] = Session_id.length() > 0;
    ep["has_token"] = TokenEnphase.length() > 50;
    ep["pact_prod_w"] = enphase_production_w;
    ep["pact_conso_w"] = enphase_house_active_w;
  }

  if (eff == SourceId::ShellyEm) {
    JsonObject sh = doc["shelly_em"].to<JsonObject>();
    sh["raw"] = ShEm_rawData;
    sh["poll_count"] = shellyEmPollCounter;
  }

  if (eff == SourceId::SmartG) {
    JsonObject sg = doc["smartg"].to<JsonObject>();
    sg["raw"] = SG_rawData;
  }

  if (eff == SourceId::HomeW) {
    JsonObject hw = doc["homewizard"].to<JsonObject>();
    hw["raw"] = HW_rawData;
  }

  if (eff == SourceId::ShellyPro) {
    JsonObject sp = doc["shelly_pro"].to<JsonObject>();
    sp["raw"] = ShPro_rawData;
  }

  if (eff == SourceId::JsyMk333) {
    JsonObject j3 = doc["jsy333"].to<JsonObject>();
    j3["raw"] = MK333_rawData;
    j3["serial_baud"] = JsyMk333SerialBaud;
  }

  if (balansun_active_source_get() == SourceId::Pmqtt) {
    JsonObject pm = doc["pmqtt"].to<JsonObject>();
    pm["topic"] = PmqttTopic;
    pm["schema"] = PmqttSchema;
    pm["last_pw"] = PwMQTT_last;
  }

  if (balansun_active_source_get() == SourceId::BalansunPeer) {
    JsonObject ext = doc["ext"].to<JsonObject>();
    ext["ext_peer_ip"] = ip32ToDotted(ext_peer_ip);
    ext["ext_peer_port"] = ext_peer_port;
    ext["ext_peer_path"] = ext_peer_path;
    ext["ext_protocol"] = ext_peer_protocol.length() ? ext_peer_protocol : "json";
    ext["last_poll_ok"] = ext_peer_last_poll_ok;
    ext["last_poll_ms_ago"] =
        (ext_peer_last_poll_ms > 0) ? (int)((millis() - ext_peer_last_poll_ms) & 0x7FFFFFFF) : -1;
    ext["last_error"] = ext_peer_last_poll_err;
    ext["last_frame_preview"] = ext_peer_last_poll_preview;
    ext["protocol_used"] = ext_peer_last_poll_protocol.length() ? ext_peer_last_poll_protocol : "json";
  }
}

void balansun_sources_brute_panel_json(String &out) {
  JsonDocument d;
  const SourceId eff = balansun_effective_meter_id();
  d["panel"] = balansun_source_wire_for_id(eff);
  JsonObject p = d["panels"].to<JsonObject>();
  p["analog"] = (eff == SourceId::Analog);
  p["jsy_mk194"] = (eff == SourceId::JsyMk194);
  p["jsy_mk333"] = (eff == SourceId::JsyMk333);
  p["linky"] = (eff == SourceId::Linky);
  p["enphase"] = (eff == SourceId::Enphase);
  p["smartg"] = (eff == SourceId::SmartG);
  p["shelly_em"] = (eff == SourceId::ShellyEm);
  p["homewizard"] = (eff == SourceId::HomeW);
  p["shelly_pro"] = (eff == SourceId::ShellyPro);
  p["pmqtt"] = (eff == SourceId::Pmqtt);
  p["notdef"] = (eff == SourceId::NotDef);
  serializeJson(d, out);
}

bool balansun_cap_mqtt_triac_channel_block_for(SourceId id) {
  return balansun_source_logic_cap_mqtt_triac_channel_block_for(id);
}

bool balansun_cap_mqtt_triac_channel_block() {
  return balansun_cap_mqtt_triac_channel_block_for(g_active_source);
}

bool balansun_cap_mqtt_linky_tariff_for(SourceId id) {
  return balansun_source_logic_cap_mqtt_linky_tariff_for(id, tempoRteEnabled);
}

bool balansun_cap_mqtt_linky_tariff() { return balansun_cap_mqtt_linky_tariff_for(g_active_source); }

bool balansun_cap_serial_adc_gpio_restrict_for(SourceId id) {
  return balansun_source_logic_cap_serial_adc_gpio_restrict_for(id);
}

bool balansun_cap_serial_adc_gpio_restrict() {
  return balansun_cap_serial_adc_gpio_restrict_for(g_active_source);
}
