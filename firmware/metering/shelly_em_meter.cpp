/*
 * shelly_em_meter.cpp — Source ShellyEm: HTTP to Shelly EM Gen1 (REST energy meters).
 * See: /en/hardware-pinout/ — source_shellyem; GUIDE A.6.
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_SHELLY_EM
#include "balansun_globals.h"
#include "api_util.h"
#include "balansun_lan_http_client.h"
#include "json_field_parse.h"
#include "shelly_em_logic.h"
// --- Shelly EM HTTP client (channel 0/1 monophase or three-phase status) ---
void shelly_em_poll() {
  String S = "";
  String Shelly_Data = "";
  float Pw = 0;
  float voltage = 0;
  float pf = 0;


  const String host = ip32ToDotted(ext_peer_ip);
  int voie = meter_channel.toInt();
  int Voie = voie % 2;

  if (shellyEmPollCounter == 1) {
    Voie = (Voie + 1) % 2;
  }
  String url = "/emeter/" + String(Voie);
  if (voie == 3) url = "/status";  // three-phase
  shellyEmPollCounter = (shellyEmPollCounter + 1) % 5;  // poll alternate channel 1 in 6 (second EM input unused by router)
  if (!balansun_lan_http_get(host, 80, url, Shelly_Data)) {
    Serial.println("connection to client Shelly Em failed (call from shelly_em_poll)");
    delay(200);
    meterPeerFailures++;
    return;
  }
  int p = Shelly_Data.indexOf("{");
  Shelly_Data = Shelly_Data.substring(p);
  if (voie == 3) {  // three-phase
    ShEm_rawData = "<strong>Three-phase</strong><br>" + Shelly_Data;
    p = Shelly_Data.indexOf("emeters");
    Shelly_Data = Shelly_Data.substring(p + 10);
    Pw = parse_json_float("power", Shelly_Data);  //Phase 1
    pf = parse_json_float("pf", Shelly_Data);
    pf = abs(pf);
    float total_Pw = Pw;
    float total_Pva = 0;
    if (pf > 0) {
      total_Pva = abs(Pw) / pf;
    }
    float total_energy_import = parse_json_float("total\"", Shelly_Data);
    float total_E_injecte = parse_json_float("total_returned", Shelly_Data);
    p = Shelly_Data.indexOf("}");
    Shelly_Data = Shelly_Data.substring(p + 1);
    Pw = parse_json_float("power", Shelly_Data);  //Phase 2
    pf = parse_json_float("pf", Shelly_Data);
    pf = abs(pf);
    total_Pw += Pw;
    if (pf > 0) {
      total_Pva += abs(Pw) / pf;
    }
    total_energy_import += parse_json_float("total\"", Shelly_Data);
    total_E_injecte += parse_json_float("total_returned", Shelly_Data);
    p = Shelly_Data.indexOf("}");
    Shelly_Data = Shelly_Data.substring(p + 1);
    Pw = parse_json_float("power", Shelly_Data);  //Phase 3
    pf = parse_json_float("pf", Shelly_Data);
    pf = abs(pf);
    total_Pw += Pw;
    if (pf > 0) {
      total_Pva += abs(Pw) / pf;
    }
    total_energy_import += parse_json_float("total\"", Shelly_Data);
    total_E_injecte += parse_json_float("total_returned", Shelly_Data);
    house_energy_import_wh = int(total_energy_import);
    house_energy_export_wh = int(total_E_injecte);
    if (total_Pw > 0) {
      house_active_import_w = int(total_Pw);
      house_active_export_w = 0;
      house_apparent_import_va = int(total_Pva);
      house_apparent_export_va = 0;
    } else {
      house_active_import_w = 0;
      house_active_export_w = -int(total_Pw);
      house_apparent_export_va = int(total_Pva);
      house_apparent_import_va = 0;
    }
  } else {
    ShEm_rawData = "<strong>Channel: " + String(voie) + "</strong><br>" + Shelly_Data;
    ShellyEmMonoReading rd;
    if (shelly_em_logic_parse_monophase_json(Shelly_Data.c_str(), rd)) {
      Pw = rd.power_w;
      voltage = rd.voltage_v;
      pf = rd.pf;
      if (Voie == voie) {
        house_active_import_w = rd.active_import_w;
        house_active_export_w = rd.active_export_w;
        house_apparent_import_va = rd.apparent_import_va;
        house_apparent_export_va = rd.apparent_export_va;
        house_energy_import_wh = rd.energy_import_wh;
        house_energy_export_wh = rd.energy_export_wh;
        house_power_factor = pf;
        house_voltage_v = voltage;
      } else {
        second_active_import_w = rd.active_import_w;
        second_active_export_w = rd.active_export_w;
        second_apparent_import_va = rd.apparent_import_va;
        second_apparent_export_va = rd.apparent_export_va;
        second_energy_import_wh = rd.energy_import_wh;
        second_energy_export_wh = rd.energy_export_wh;
        second_power_factor = pf;
        second_voltage_v = voltage;
      }
    }
  }

  esp_task_wdt_reset();  // WDT reset on each Shelly frame
  if (shellyEmPollCounter > 1) meter_reading_valid = true;
  if (cptLEDyellow > 30) {
    cptLEDyellow = 4;
  }
}

#endif /* BALANSUN_ENABLE_SOURCE_SHELLY_EM */
