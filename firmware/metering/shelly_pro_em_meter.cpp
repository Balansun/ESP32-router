/*
 * shelly_pro_em_meter.cpp — Source ShellyPro: HTTP/RPC Shelly Pro EM (multi-channel).
 * See: /en/hardware-pinout/ §17 recap; GUIDE A.6.
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_SHELLY_PRO
#include "balansun_globals.h"
#include "json_field_parse.h"
#include "api_util.h"
#include "balansun_lan_http_client.h"
// Shelly Pro EM — RPC client (shellypro3em three-phase when meter_channel = 3).

static String ShellyPro_Name = "";
static String ShellyPro_Profile = "";

void shelly_pro_em_poll(void) {
  const String host = ip32ToDotted(ext_peer_ip);
  int voie = meter_channel.toInt();

  if (ShellyPro_Name.length() == 0) {
    String Shelly_Data;
    if (!balansun_lan_http_get(host, 80, "/rpc/Shelly.GetDeviceInfo", Shelly_Data)) {
      delay(200);
      meterPeerFailures++;
      return;
    }
    ShellyPro_Name = parse_json_string("id", Shelly_Data);
    int p = ShellyPro_Name.indexOf("-");
    if (p > 0) {
      ShellyPro_Name = ShellyPro_Name.substring(0, p);
    }
    ShellyPro_Profile = parse_json_string("profile", Shelly_Data);
  }

  shellyEmPollCounter = (shellyEmPollCounter + 1) % 5;
  String Shelly_Data;
  if (!balansun_lan_http_get(host, 80, "/rpc/Shelly.GetStatus", Shelly_Data)) {
    delay(200);
    meterPeerFailures++;
    return;
  }
  int p = Shelly_Data.indexOf("{");
  if (p < 0) return;
  Shelly_Data = Shelly_Data.substring(p);
  if (Shelly_Data.length() > 0 && Shelly_Data.charAt(Shelly_Data.length() - 1) != '}') {
    Shelly_Data += "}";
  }

  if (ShellyPro_Name.indexOf("shellypro3em") == 0 && voie == 3) {
    String tmp = prefilter_json("em:0", ":", Shelly_Data);
    float Pw = parse_json_float("total_act_power", tmp);
    float T1 = parse_json_float("a_voltage", tmp);
    float T2 = parse_json_float("b_voltage", tmp);
    float T3 = parse_json_float("c_voltage", tmp);
    float pf1 = fabsf(parse_json_float("a_pf", tmp));
    float pf2 = fabsf(parse_json_float("b_pf", tmp));
    float pf3 = fabsf(parse_json_float("c_pf", tmp));
    float voltage = (T1 + T2 + T3) / 3.0f;
    float pf = abs((pf1 + pf2 + pf3) / 3.0f);
    if (pf > 1.0f) pf = 1.0f;
    if (Pw >= 0) {
      house_active_import_w = (int)Pw;
      house_active_export_w = 0;
      house_apparent_import_va = (pf > 0.01f) ? (int)(Pw / pf) : 0;
      house_apparent_export_va = 0;
    } else {
      house_active_import_w = 0;
      house_active_export_w = (int)(-Pw);
      house_apparent_export_va = (pf > 0.01f) ? (int)((-Pw) / pf) : 0;
      house_apparent_import_va = 0;
    }
    tmp = prefilter_json("emdata:0", ":", Shelly_Data);
    house_energy_import_wh = (long)parse_json_float("total_act", tmp);
    house_energy_export_wh = (long)parse_json_float("total_act_ret", tmp);
    house_power_factor = pf;
    house_voltage_v = voltage;
    ShPro_rawData = "<strong>" + ShellyPro_Name + "</strong><br>" + Shelly_Data;
  } else {
    ShPro_rawData = "<strong>" + ShellyPro_Name + "</strong> profil=" + ShellyPro_Profile + " voie=" + String(voie) +
                      "<br><em>Note: utiliser voie 3 (tri) pour shellypro3em, ou source ShellyEm pour Gen1.</em><br>" + Shelly_Data;
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

#endif /* BALANSUN_ENABLE_SOURCE_SHELLY_PRO */
