/*
 * enphase_envoy_meter.cpp — Source Enphase: HTTPS Envoy API (token/session, Wh counters).
 * See: /en/hardware-pinout/ — source_enphase; GUIDE A.6.
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_ENPHASE
#include "enphase_envoy_meter.h"

#include "api_util.h"
#include "balansun_globals.h"
#include "json_field_parse.h"
#include "UrlEncode.h"

#include <WiFiClientSecure.h>
#include <esp_task_wdt.h>

static WiFiClientSecure clientSecu;

void EnphaseMeter::setup() {
  const char *server1Enphase = "enlighten.enphaseenergy.com";
  String host = String(server1Enphase);
  String adrEnphase = "https://" + host + "/login/login.json";
  String requestBody = "user[email]=" + EnphaseUser + "&user[password]=" + urlEncode(EnphasePwd);

  if (EnphaseUser != "" && EnphasePwd != "") {
    Serial.println("Trying Enlighten server 1 for session_id...");
    Debug.println("Trying Enlighten server 1 for session_id...");
    clientSecu.setInsecure();
    if (!clientSecu.connect(server1Enphase, 443)) {
      Serial.println("Connection failed to Enlighten server :" + host);
    } else {
      Serial.println("Connected to Enlighten server:" + host);
      clientSecu.println("POST " + adrEnphase + "?" + requestBody + " HTTP/1.0");
      clientSecu.println("Host: " + host);
      clientSecu.println("Connection: close");
      clientSecu.println();
      String line = "";
      while (clientSecu.connected()) {
        line = clientSecu.readStringUntil('\n');
        if (line == "\r") {
          Serial.println("headers 1 Enlighten received");
          JsonToken = "";
        }
        JsonToken += line;
      }
      while (clientSecu.available()) {
        char c = clientSecu.read();
        Serial.write(c);
      }
      clientSecu.stop();
    }
    Session_id = parse_json_string("session_id", String(JsonToken.c_str()));
    Serial.println("session_id :" + Session_id);
    Debug.println("session_id :" + Session_id);
  } else {
    Serial.println("Connecting to Envoy-S gateway (firmware v5)");
    Debug.println("Connecting to Envoy-S gateway (firmware v5)");
  }

  if (Session_id != "" && meter_channel != "" && EnphaseUser != "") {
    const char *server2Enphase = "entrez.enphaseenergy.com";
    host = String(server2Enphase);
    adrEnphase = "https://" + host + "/tokens";
    requestBody = "{\"session_id\":\"" + Session_id + "\", \"serial_num\":" + meter_channel +
                  ", \"username\":\"" + EnphaseUser + "\"}";
    Serial.println("Trying Enlighten server 2 for token...");
    Debug.println("Trying Enlighten server 2 for token...");
    clientSecu.setInsecure();
    if (!clientSecu.connect(server2Enphase, 443)) {
      Serial.println("Connection failed to :" + host);
    } else {
      Serial.println("Connected to :" + host);
      clientSecu.println("POST " + adrEnphase + " HTTP/1.0");
      clientSecu.println("Host: " + host);
      clientSecu.println("Content-Type: application/json");
      clientSecu.println("content-length:" + String(requestBody.length()));
      clientSecu.println("Connection: close");
      clientSecu.println();
      clientSecu.println(requestBody);
      clientSecu.println();
      Serial.println("Waiting for Enlighten response (token request)...");
      String line = "";
      JsonToken = "";
      while (clientSecu.connected()) {
        line = clientSecu.readStringUntil('\n');
        if (line == "\r") {
          Serial.println("headers 2 enlighten received");
          JsonToken = "";
        }
        JsonToken += line;
      }
      while (clientSecu.available()) {
        char c = clientSecu.read();
        Serial.write(c);
      }
      clientSecu.stop();
      JsonToken.trim();
      Serial.println("Token :" + JsonToken);
      Debug.println("Token :" + JsonToken);
      if (JsonToken.length() > 50) {
        TokenEnphase = JsonToken;
        metering_task_ms_min = 1000;
        metering_task_ms_max = 1;
        metering_task_ms_avg = 1;
        last_metering_task_at_ms = millis();
        last_metering_task_ms = millis();
        poll_period_ms = 1000;
      }
    }
  }
}

void EnphaseMeter::poll() {
  const int num_port_iq = 443;
  String json_enphase = "";
  const String host = ip32ToDotted(ext_peer_ip);

  if (TokenEnphase.length() > 50 && EnphaseUser != "") {
    if (millis() > 2592000000UL) {
      setup();
    }

    clientSecu.setInsecure();
    if (!clientSecu.connect(host.c_str(), num_port_iq)) {
      Serial.println("Connection failed to Envoy-S server!");
    } else {
      clientSecu.println("GET https://" + host + "/ivp/meters/reports/consumption HTTP/1.0");
      clientSecu.println("Host: " + host);
      clientSecu.println("Accept: application/json");
      clientSecu.println("Authorization: Bearer " + TokenEnphase);
      clientSecu.println("Connection: close");
      clientSecu.println();

      String line = "";
      while (clientSecu.connected()) {
        line = clientSecu.readStringUntil('\n');
        if (line == "\r") {
          json_enphase = "";
        }
        json_enphase += line;
      }
      while (clientSecu.available()) {
        char c = clientSecu.read();
        Serial.write(c);
      }
      clientSecu.stop();
    }
  } else {
    WiFiClient clientFirmV5;
    if (!clientFirmV5.connect(host.c_str(), 80)) {
      Serial.println("connection to client clientFirmV5 failed (call to Envoy-S)");
      markHttpFailure();
      return;
    }
    const String url = "/ivp/meters/reports/consumption";
    clientFirmV5.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" +
                       "Connection: close\r\n\r\n");
    unsigned long timeout = millis();
    while (clientFirmV5.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println(">>> client clientFirmV5 Timeout !");
        clientFirmV5.stop();
        return;
      }
    }
    timeout = millis();
    String line;
    while (clientFirmV5.available() && (millis() - timeout < 5000)) {
      line = clientFirmV5.readStringUntil('\n');
      if (line == "\r") {
        json_enphase = "";
      }
      json_enphase += line;
    }
  }

  const String tot_conso = prefilter_json("total-consumption", "cumulative", json_enphase);
  enphase_house_active_w = int(parse_json_float("actPower", tot_conso));
  const String net_conso = prefilter_json("net-consumption", "cumulative", json_enphase);
  const float pact_reseau = parse_json_float("actPower", net_conso);
  if (pact_reseau < 0) {
    house_active_import_w = 0;
    house_active_export_w = int(-pact_reseau);
  } else {
    house_active_export_w = 0;
    house_active_import_w = int(pact_reseau);
  }
  const float pva_reseau = parse_json_float("apprntPwr", net_conso);
  if (pva_reseau < 0) {
    house_apparent_import_va = 0;
    house_apparent_export_va = int(-pva_reseau);
  } else {
    house_apparent_export_va = 0;
    house_apparent_import_va = int(pva_reseau);
  }
  float power_factor = 0;
  if (pva_reseau != 0) {
    power_factor = floor(100 * pact_reseau / pva_reseau) / 100;
    power_factor = min(power_factor, float(1));
  }
  house_power_factor = power_factor;
  const long wh_dlvd_cum = parse_json_long("whDlvdCum", net_conso);
  long delta_wh = 0;
  if (wh_dlvd_cum != 0) {
    if (LastwhDlvdCum == 0) {
      LastwhDlvdCum = wh_dlvd_cum;
    }
    delta_wh = wh_dlvd_cum - LastwhDlvdCum;
    LastwhDlvdCum = wh_dlvd_cum;
    if (delta_wh < 0) {
      house_energy_export_wh = house_energy_export_wh - delta_wh;
    } else {
      house_energy_import_wh = house_energy_import_wh + delta_wh;
    }
  }
  house_voltage_v = parse_json_float("rmsVoltage", net_conso);
  house_current_a = parse_json_float("rmsCurrent", net_conso);
  enphase_production_w = enphase_house_active_w - int(pact_reseau);
  meter_reading_valid = true;
  if (pact_reseau != 0 || pva_reseau != 0) {
    esp_task_wdt_reset();
  }
  if (cptLEDyellow > 30) {
    cptLEDyellow = 4;
  }
}

void EnphaseMeter::appendDiagnostics(JsonObject doc, int linky_tail_max) {
  (void)linky_tail_max;
  JsonObject ep = doc["enphase"].to<JsonObject>();
  ep["user"] = EnphaseUser;
  ep["serial"] = meter_channel;
  ep["has_user"] = EnphaseUser.length() > 0;
  ep["has_session"] = Session_id.length() > 0;
  ep["has_token"] = TokenEnphase.length() > 50;
  ep["pact_prod_w"] = enphase_production_w;
  ep["pact_conso_w"] = enphase_house_active_w;
}

IMeterDriver *balansun_meter_instance_enphase() {
  static EnphaseMeter instance;
  return &instance;
}

#endif /* BALANSUN_ENABLE_SOURCE_ENPHASE */
