/*
 * homewizard_meter.cpp — Source HomeW: HTTP API HomeWizard P1 / energy socket.
 * See: /en/hardware-pinout/ §17 recap; GUIDE A.6.
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_HOMEW
#include "balansun_globals.h"
#include "api_util.h"
#include "balansun_lan_http_client.h"
#include "json_flat_meter_logic.h"
// HomeWizard — same flat JSON pattern as SmartG on ext_peer_ip:80 /api/v1/data

void homewizard_poll(void) {
  const String host = ip32ToDotted(ext_peer_ip);
  String HomeW_Data;
  if (!balansun_lan_http_get(host, 80, "/api/v1/data", HomeW_Data)) {
    delay(200);
    meterPeerFailures++;
    return;
  }
  int p = HomeW_Data.indexOf("{");
  if (p < 0) return;
  HomeW_Data = HomeW_Data.substring(p + 1);
  p = HomeW_Data.indexOf("}");
  if (p < 0) return;
  HomeW_Data = HomeW_Data.substring(0, p);
  JsonFlatMeterReading rd;
  if (!json_flat_meter_logic_parse_homewizard(HomeW_Data.c_str(), rd)) {
    return;
  }
  house_active_import_w = rd.active_import_w;
  house_active_export_w = rd.active_export_w;
  house_energy_import_wh = rd.energy_import_wh;
  house_energy_export_wh = rd.energy_export_wh;
  HW_rawData = HomeW_Data;
  house_voltage_v = 230.0f;
  house_power_factor = 1.0f;
  meter_reading_valid = true;
  esp_task_wdt_reset();
  if (cptLEDyellow > 30) {
    cptLEDyellow = 4;
  }
}

#endif /* BALANSUN_ENABLE_SOURCE_HOMEW */
