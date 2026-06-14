/*
 * shelly_pro_em_meter.cpp — Source ShellyPro: HTTP/RPC Shelly Pro EM (multi-channel).
 * See: /en/hardware-pinout/ §17 recap; GUIDE A.6.
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_SHELLY_PRO
#include "shelly_pro_em_meter.h"

#include "api_util.h"
#include "balansun_globals.h"
#include "balansun_lan_http_client.h"
#include "json_field_parse.h"
#include "shelly_pro_logic.h"

#include <esp_task_wdt.h>

void ShellyProMeter::buildRequest(String &host_out, String &path_out) {
  host_out = ip32ToDotted(ext_peer_ip);
  if (!device_info_loaded_) {
    path_out = "/rpc/Shelly.GetDeviceInfo";
    return;
  }
  path_out = "/rpc/Shelly.GetStatus";
}

void ShellyProMeter::poll() {
  String host;
  String path;
  buildRequest(host, path);
  String body;
  if (!balansun_lan_http_get(host, httpPort(), path, body)) {
    markHttpFailure();
    return;
  }
  if (!device_info_loaded_) {
    device_name_ = parse_json_string("id", body);
    const int p = device_name_.indexOf("-");
    if (p > 0) {
      device_name_ = device_name_.substring(0, p);
    }
    device_profile_ = parse_json_string("profile", body);
    device_info_loaded_ = true;
    return;
  }
  shellyEmPollCounter = (shellyEmPollCounter + 1) % 5;
  if (!parseAndApply(body)) {
    return;
  }
  esp_task_wdt_reset();
  if (shellyEmPollCounter > 1) {
    meter_reading_valid = true;
  }
  if (cptLEDyellow > 30) {
    cptLEDyellow = 4;
  }
}

bool ShellyProMeter::parseAndApply(const String &body) {
  String shelly_data = body;
  const int voie = meter_channel.toInt();
  int p = shelly_data.indexOf("{");
  if (p < 0) {
    return false;
  }
  shelly_data = shelly_data.substring(p);
  if (shelly_data.length() > 0 && shelly_data.charAt(shelly_data.length() - 1) != '}') {
    shelly_data += "}";
  }
  String raw_out;
  if (shelly_pro_logic_apply_three_phase_status(shelly_data, device_name_, voie, raw_out)) {
    ShPro_rawData = raw_out;
    return true;
  }
  ShPro_rawData = "<strong>" + device_name_ + "</strong> profil=" + device_profile_ + " voie=" + String(voie) +
                  "<br><em>Note: utiliser voie 3 (tri) pour shellypro3em, ou source ShellyEm pour Gen1.</em><br>" +
                  shelly_data;
  return false;
}

void ShellyProMeter::appendDiagnostics(JsonObject doc, int linky_tail_max) {
  (void)linky_tail_max;
  JsonObject sp = doc["shelly_pro"].to<JsonObject>();
  sp["raw"] = ShPro_rawData;
}

IMeterDriver *balansun_meter_instance_shelly_pro() {
  static ShellyProMeter instance;
  return &instance;
}

#endif /* BALANSUN_ENABLE_SOURCE_SHELLY_PRO */
