/*
 * external_peer_meter.cpp — Source Ext: HTTP client to Balansun peer (ext_peer_ip, port, path).
 * Poll cycle: JSON GET /api/v1/measurements (or configured ext_peer_path).
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_EXT
#include "balansun_globals.h"
#include "api_util.h"
#include "external_peer_logic.h"
#include "balansun_lan_http_client.h"

#include <cstring>

static void ext_peer_record_poll(bool ok, const char *errTag, const String &buf, const char *protocol) {
  ext_peer_last_poll_ms = millis();
  ext_peer_last_poll_ok = ok;
  ext_peer_last_poll_err = errTag ? String(errTag) : String("");
  ext_peer_last_poll_protocol = protocol ? String(protocol) : String("");
  ext_peer_last_poll_preview = "";
  if (buf.length() == 0) return;
  String body = buf;
  body.trim();
  if (body.length() > 72) body = body.substring(0, 72);
  ext_peer_last_poll_preview = body;
}

String ext_peer_main_data_path() {
  const char *path = ext_peer_path.c_str();
  return (ext_peer_path.length() > 0 && path[0] == '/') ? String(path) : String("/api/v1/measurements");
}

String ext_peer_raw_data_path(int lastIdx) {
  return "/api/v1/sources/brute_panel?idx=" + String(lastIdx);
}

static void extPeerApplyReading(const ExternalPeerReading &rd) {
  Source_data = "JsyMk194";
  if (!tempoRteEnabled) {
    if (!rd.header_ltarf.empty()) LTARF = String(rd.header_ltarf.c_str());
    if (!rd.header_stge.empty()) STGEt = String(rd.header_stge.c_str());
  }
  house_active_import_w = rd.house_active_import_w;
  house_active_export_w = rd.house_active_export_w;
  house_apparent_import_va = rd.house_apparent_import_va;
  house_apparent_export_va = rd.house_apparent_export_va;
  house_day_energy_import_wh = rd.house_day_energy_import_wh;
  house_day_energy_export_wh = rd.house_day_energy_export_wh;
  house_energy_import_wh = rd.house_energy_import_wh;
  house_energy_export_wh = rd.house_energy_export_wh;
  second_active_import_w = rd.second_active_import_w;
  second_active_export_w = rd.second_active_export_w;
  esp_task_wdt_reset();
  cptLEDyellow = 4;
  meter_reading_valid = true;
}

void external_peer_poll() {
  String host = ip32ToDotted(ext_peer_ip);
  unsigned int port = ext_peer_port;
  if (port == 0 || port > 65535u) port = 80;

  const String path = ext_peer_main_data_path();
  String body;
  if (!balansun_lan_http_get(host, (uint16_t)port, path, body)) {
    Serial.println("Ext peer HTTP connect failed (external_peer_poll)");
    ext_peer_record_poll(false, "connect", "", nullptr);
    delay(200);
    meterPeerFailures++;
    return;
  }

  ExternalPeerReading rd;
  if (external_peer_logic_parse_measurements_json(std::string(body.c_str()), rd) && rd.valid) {
    extPeerApplyReading(rd);
    ext_peer_record_poll(true, "", body, "json");
    return;
  }

  ext_peer_record_poll(false, "parse", body, "json");
}

#endif /* BALANSUN_ENABLE_SOURCE_EXT */
