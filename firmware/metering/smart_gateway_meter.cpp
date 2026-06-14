/*
 * smart_gateway_meter.cpp — Source SmartG: HTTP JSON from Smart Gateways device.
 * See: /en/hardware-pinout/ — source_smartg; GUIDE A.6.
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_SMARTG
#include "balansun_globals.h"
#include "api_util.h"
#include "balansun_lan_http_client.h"
#include "json_flat_meter_logic.h"
// ******************************
// * Client d'un Smart Gateways *
// ******************************

void smart_gateway_poll() {
  String S = "";
  String SmartG_Data = "";
  String Gr[4];
  String data_[20];


  const String host = ip32ToDotted(ext_peer_ip);
  if (!balansun_lan_http_get(host, 82, "/smartmeter/api/read", SmartG_Data)) {
    Serial.println("connection to client SmartGateways failed (call from smart_gateway_poll)");
    delay(200);
    meterPeerFailures++;
    return;
  }
  int p = SmartG_Data.indexOf("{");
  SmartG_Data = SmartG_Data.substring(p + 1);
  p = SmartG_Data.indexOf("}");
  SmartG_Data = SmartG_Data.substring(0, p);
  JsonFlatMeterReading rd;
  if (!json_flat_meter_logic_parse_smartg(SmartG_Data.c_str(), rd)) {
    return;
  }
  house_active_import_w = rd.active_import_w;
  house_active_export_w = rd.active_export_w;
  house_energy_import_wh = rd.energy_import_wh;
  house_energy_export_wh = rd.energy_export_wh;
  SG_rawData=SmartG_Data;
  esp_task_wdt_reset();  // WDT reset on each SmartGateways frame
  meter_reading_valid = true;
  if (cptLEDyellow > 30) {
    cptLEDyellow = 4;
  }
}

#endif /* BALANSUN_ENABLE_SOURCE_SMARTG */
