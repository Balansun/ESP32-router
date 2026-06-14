/*
 * shelly_em_meter.cpp — Source ShellyEm: HTTP to Shelly EM Gen1 (REST energy meters).
 * See: /en/hardware-pinout/ — source_shellyem; GUIDE A.6.
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_SHELLY_EM
#include "shelly_em_meter.h"

#include "api_util.h"
#include "balansun_globals.h"
#include "balansun_lan_http_client.h"
#include "json_field_parse.h"
#include "shelly_em_logic.h"

#include <esp_task_wdt.h>

void ShellyEmMeter::buildRequest(String &host_out, String &path_out) {
  host_out = ip32ToDotted(ext_peer_ip);
  const int voie = meter_channel.toInt();
  polled_channel_ = voie % 2;
  if (shellyEmPollCounter == 1) {
    polled_channel_ = (polled_channel_ + 1) % 2;
  }
  path_out = "/emeter/" + String(polled_channel_);
  if (voie == 3) {
    path_out = "/status";
  }
  shellyEmPollCounter = (shellyEmPollCounter + 1) % 5;
}

void ShellyEmMeter::poll() {
  String host;
  String path;
  buildRequest(host, path);
  String body;
  if (!balansun_lan_http_get(host, httpPort(), path, body)) {
    Serial.println("connection to client Shelly Em failed (call from shelly_em_poll)");
    markHttpFailure();
    return;
  }
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

bool ShellyEmMeter::parseAndApply(const String &body) {
  String shelly_data = body;
  const int voie = meter_channel.toInt();
  int p = shelly_data.indexOf("{");
  shelly_data = shelly_data.substring(p);
  if (voie == 3) {
    ShEm_rawData = "<strong>Three-phase</strong><br>" + shelly_data;
    p = shelly_data.indexOf("emeters");
    shelly_data = shelly_data.substring(p + 10);
    float pw = parse_json_float("power", shelly_data);
    float pf = parse_json_float("pf", shelly_data);
    pf = abs(pf);
    float total_pw = pw;
    float total_pva = 0;
    if (pf > 0) {
      total_pva = abs(pw) / pf;
    }
    float total_energy_import = parse_json_float("total\"", shelly_data);
    float total_e_injecte = parse_json_float("total_returned", shelly_data);
    p = shelly_data.indexOf("}");
    shelly_data = shelly_data.substring(p + 1);
    pw = parse_json_float("power", shelly_data);
    pf = parse_json_float("pf", shelly_data);
    pf = abs(pf);
    total_pw += pw;
    if (pf > 0) {
      total_pva += abs(pw) / pf;
    }
    total_energy_import += parse_json_float("total\"", shelly_data);
    total_e_injecte += parse_json_float("total_returned", shelly_data);
    p = shelly_data.indexOf("}");
    shelly_data = shelly_data.substring(p + 1);
    pw = parse_json_float("power", shelly_data);
    pf = parse_json_float("pf", shelly_data);
    pf = abs(pf);
    total_pw += pw;
    if (pf > 0) {
      total_pva += abs(pw) / pf;
    }
    total_energy_import += parse_json_float("total\"", shelly_data);
    total_e_injecte += parse_json_float("total_returned", shelly_data);
    house_energy_import_wh = int(total_energy_import);
    house_energy_export_wh = int(total_e_injecte);
    if (total_pw > 0) {
      house_active_import_w = int(total_pw);
      house_active_export_w = 0;
      house_apparent_import_va = int(total_pva);
      house_apparent_export_va = 0;
    } else {
      house_active_import_w = 0;
      house_active_export_w = -int(total_pw);
      house_apparent_export_va = int(total_pva);
      house_apparent_import_va = 0;
    }
    return true;
  }

  ShEm_rawData = "<strong>Channel: " + String(voie) + "</strong><br>" + shelly_data;
  ShellyEmMonoReading rd;
  if (!shelly_em_logic_parse_monophase_json(shelly_data.c_str(), rd)) {
    return false;
  }
  const float pf = rd.pf;
  const float voltage = rd.voltage_v;
  if (polled_channel_ == voie) {
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
  return true;
}

void ShellyEmMeter::appendDiagnostics(JsonObject doc, int linky_tail_max) {
  (void)linky_tail_max;
  JsonObject sh = doc["shelly_em"].to<JsonObject>();
  sh["raw"] = ShEm_rawData;
  sh["poll_count"] = shellyEmPollCounter;
}

IMeterDriver *balansun_meter_instance_shelly_em() {
  static ShellyEmMeter instance;
  return &instance;
}

#endif /* BALANSUN_ENABLE_SOURCE_SHELLY_EM */
