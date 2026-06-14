/*
 * pmqtt_meter.cpp — Source Pmqtt: subscribe PmqttTopic on MQTTIP broker, parse PmqttSchema JSON.
 * User: /fr/user-guide/#guide-a7-mqtt-comme-source-de-mesure-pmqtt
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_PMqtt
#include "pmqtt_meter.h"

#include "balansun_globals.h"
#include "balansun_pub.h"
#include "balansun_meter_json.h"
#include "pmqtt_bindings.h"
#include <ArduinoJson.h>
#include "balansun_json.h"

#include <esp_task_wdt.h>

void ApplyPmqttJsonPayload(const String &msg) {
  BalansunJsonDoc _balansunJsonPool1 = balansun_json_doc_alloc(2048);
  JsonDocument &doc = _balansunJsonPool1;
  DeserializationError e = deserializeJson(doc, msg);
  if (e) {
    return;
  }
  if (!doc["house"].isNull() && doc["house"].is<JsonObject>()) {
    String err;
    if (ApplyMeterSnapshotFromJson(doc.as<JsonObject>(), &err)) {
      LastPwMQTTMillis = millis();
      if (!doc["Pw"].isNull()) {
        PwMQTT_last = doc["Pw"].as<float>();
      } else if (!doc["power_w"].isNull()) {
        PwMQTT_last = doc["power_w"].as<float>();
      } else if (!doc["active_power_w"].isNull()) {
        PwMQTT_last = doc["active_power_w"].as<float>();
      } else {
        PwMQTT_last = (float)(house_active_import_w - house_active_export_w);
      }
      meter_reading_valid = true;
      BalansunPublishFromGlobals();
      return;
    }
  }
  float pw = 0;
  bool have = false;
  if (PmqttSchema.indexOf("Pw") >= 0 && !doc["Pw"].isNull()) {
    pw = doc["Pw"].as<float>();
    have = true;
  }
  if (!have && !doc["power_w"].isNull()) {
    pw = doc["power_w"].as<float>();
    have = true;
  }
  if (!have && !doc["active_power_w"].isNull()) {
    pw = doc["active_power_w"].as<float>();
    have = true;
  }
  if (!have) {
    return;
  }

  LastPwMQTTMillis = millis();
  PwMQTT_last = pw;
  float pf = 1.0f;
  if (PmqttSchema.indexOf("Pf") >= 0 && !doc["Pf"].isNull()) {
    pf = abs(doc["Pf"].as<float>());
    if (pf > 1.0f) {
      pf = 1.0f;
    }
  }
  if (pw >= 0) {
    house_active_import_w = (int)pw;
    house_active_export_w = 0;
    if (pf > 0.01f) {
      house_apparent_import_va = (int)(pw / pf);
    } else {
      house_apparent_import_va = house_active_import_w;
    }
    house_apparent_export_va = 0;
  } else {
    house_active_import_w = 0;
    house_active_export_w = (int)(-pw);
    if (pf > 0.01f) {
      house_apparent_export_va = (int)((-pw) / pf);
    } else {
      house_apparent_export_va = house_active_export_w;
    }
    house_apparent_import_va = 0;
  }
  meter_reading_valid = true;
  BalansunPublishFromGlobals();
  esp_task_wdt_reset();
  if (cptLEDyellow > 30) {
    cptLEDyellow = 4;
  }
}

bool ApplyPmqttBindingsPayload(const String &topic, const String &msg, String *err) {
  return pmqtt_bindings_apply_message(topic, msg, err);
}

void PmqttMeter::appendDiagnostics(JsonObject doc, int linky_tail_max) {
  (void)linky_tail_max;
  JsonObject pm = doc["pmqtt"].to<JsonObject>();
  pm["topic"] = PmqttTopic;
  pm["schema"] = PmqttSchema;
  pm["last_pw"] = PwMQTT_last;
}

IMeterDriver *balansun_meter_instance_pmqtt() {
  static PmqttMeter instance;
  return &instance;
}

#else /* !BALANSUN_ENABLE_SOURCE_PMqtt */

#include <Arduino.h>

void ApplyPmqttJsonPayload(const String &) {}
bool ApplyPmqttBindingsPayload(const String &, const String &, String *) { return false; }

#endif /* BALANSUN_ENABLE_SOURCE_PMqtt */
