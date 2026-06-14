/*
 * enphase_envoy_meter.cpp — Source Enphase: HTTPS Envoy API (token/session, Wh counters).
 * See: /en/hardware-pinout/ — source_enphase; GUIDE A.6.
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_ENPHASE
#include "balansun_globals.h"
#include "api_util.h"
#include "json_field_parse.h"
#include "UrlEncode.h"
#include <WiFiClientSecure.h>
WiFiClientSecure clientSecu;
void enphase_envoy_setup() {

  //Obtention Session ID
  //********************
  const char* server1Enphase = "enlighten.enphaseenergy.com";
  String Host = String(server1Enphase);
  String adrEnphase = "https://" + Host + "/login/login.json";
  String requestBody = "user[email]=" + EnphaseUser + "&user[password]=" + urlEncode( EnphasePwd);

  if (EnphaseUser != "" && EnphasePwd != "") {
    Serial.println("Trying Enlighten server 1 for session_id...");
    Debug.println("Trying Enlighten server 1 for session_id...");
    clientSecu.setInsecure();  //skip verification
    if (!clientSecu.connect(server1Enphase, 443))
      Serial.println("Connection failed to Enlighten server :" + Host);
    else {
      Serial.println("Connected to Enlighten server:" + Host);
      clientSecu.println("POST " + adrEnphase + "?" + requestBody + " HTTP/1.0");
      clientSecu.println("Host: " + Host);
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
      // if there are incoming bytes available
      // from the server, read them and print them:
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
  //Obtention Token
  //********************
  if (Session_id != "" && meter_channel != "" && EnphaseUser != "") {
    const char* server2Enphase = "entrez.enphaseenergy.com";
    Host = String(server2Enphase);
    adrEnphase = "https://" + Host + "/tokens";
    requestBody = "{\"session_id\":\"" + Session_id + "\", \"serial_num\":" + meter_channel + ", \"username\":\"" + EnphaseUser + "\"}";
    Serial.println("Trying Enlighten server 2 for token...");
    Debug.println("Trying Enlighten server 2 for token...");
    clientSecu.setInsecure();  //skip verification
    if (!clientSecu.connect(server2Enphase, 443))
      Serial.println("Connection failed to :" + Host);
    else {
      Serial.println("Connected to :" + Host);
      clientSecu.println("POST " + adrEnphase + " HTTP/1.0");
      clientSecu.println("Host: " + Host);
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
      // if there are incoming bytes available
      // from the server, read them and print them:
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

void enphase_envoy_poll() {  // Read house consumption from Envoy
  int Num_portIQ = 443;
  String JsonEnPhase = "";
  String host = ip32ToDotted(ext_peer_ip);

  if (TokenEnphase.length() > 50 && EnphaseUser != "") {  // HTTPS path (firmware V7+)
    if (millis() > 2592000000) {                          // Refresh token about every 30 days
      enphase_envoy_setup();
    }

    clientSecu.setInsecure();  //skip verification
    if (!clientSecu.connect(host.c_str(), Num_portIQ)) {
      Serial.println("Connection failed to Envoy-S server!");
    } else {
      //Serial.println("Connected to Envoy-S server!");
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
          //Serial.println("headers received");
          JsonEnPhase = "";
        }
        JsonEnPhase += line;
      }
      // if there are incoming bytes available
      // from the server, read them and print them:
      while (clientSecu.available()) {
        char c = clientSecu.read();
        Serial.write(c);
      }

      clientSecu.stop();
    }
  } else {  // Envoy V5: plain HTTP on port 80
    // Use WiFiClient class to create TCP connections http
    WiFiClient clientFirmV5;
    if (!clientFirmV5.connect(host.c_str(), 80)) {
      Serial.println("connection to client clientFirmV5 failed (call to Envoy-S)");
      delay(200);
      meterPeerFailures++;
      return;
    }
    String url = "/ivp/meters/reports/consumption";
    clientFirmV5.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");;
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
    // Read raw HTTP response lines from gateway
    while (clientFirmV5.available() && (millis() - timeout < 5000)) {
      line = clientFirmV5.readStringUntil('\n');
      if (line == "\r") {
          //Serial.println("headers received");
          JsonEnPhase = "";
        }
        JsonEnPhase += line;
    }
  }
 
  // Avoid ArduinoJson on large Envoy payloads; use string field extractors instead
  String TotConso = prefilter_json("total-consumption", "cumulative", JsonEnPhase);
  enphase_house_active_w = int(parse_json_float("actPower", TotConso));
  String NetConso = prefilter_json("net-consumption", "cumulative", JsonEnPhase);
  float PactReseau = parse_json_float("actPower", NetConso);
  if (PactReseau < 0) {
    house_active_import_w = 0;
    house_active_export_w = int(-PactReseau);
  } else {
    house_active_export_w = 0;
    house_active_import_w = int(PactReseau);
  }
  float PvaReseau = parse_json_float("apprntPwr", NetConso);
  if (PvaReseau < 0) {
    house_apparent_import_va = 0;
    house_apparent_export_va = int(-PvaReseau);
  } else {
    house_apparent_export_va = 0;
    house_apparent_import_va = int(PvaReseau);
  }
  float PowerFactor = 0;
  if (PvaReseau != 0) {
    PowerFactor = floor(100 * PactReseau / PvaReseau) / 100;
    PowerFactor = min(PowerFactor, float(1));
  }
  house_power_factor = PowerFactor;
  long whDlvdCum = parse_json_long("whDlvdCum", NetConso);
  long DeltaWh = 0;
  if (whDlvdCum != 0) {  // valid cumulative reading
    if (LastwhDlvdCum == 0) {
      LastwhDlvdCum = whDlvdCum;
    }
    DeltaWh = whDlvdCum - LastwhDlvdCum;
    LastwhDlvdCum = whDlvdCum;
    if (DeltaWh < 0) {
      house_energy_export_wh = house_energy_export_wh - DeltaWh;
    } else {
      house_energy_import_wh = house_energy_import_wh + DeltaWh;
    }
  }
  house_voltage_v = parse_json_float("rmsVoltage", NetConso);
  house_current_a = parse_json_float("rmsCurrent", NetConso);
  enphase_production_w = enphase_house_active_w - int(PactReseau);
  meter_reading_valid = true;
  if (PactReseau != 0 || PvaReseau != 0) {
    esp_task_wdt_reset();  // Feed WDT on each non-idle Envoy metered frame
  }
  if (cptLEDyellow > 30) {
    cptLEDyellow = 4;
  }
}

#endif /* BALANSUN_ENABLE_SOURCE_ENPHASE */
