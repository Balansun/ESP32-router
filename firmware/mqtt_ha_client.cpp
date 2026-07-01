/*
 * mqtt_ha_client.cpp — MQTT broker connection, commands, and state publish.
 */
#include "balansun_globals.h"
#include "balansun_diag.h"
#include <cstdio>
#include "balansun_board.h"
#include "balansun_device_id.h"
#include "balansun_triac_isr.h"
#include "mqtt_ha_client.h"
#include "mqtt_ha_discovery.h"
#include "mqtt_ha_topics.h"
#include "mqtt_ha_command_logic.h"
#include "api.h"
#include "balansun_source.h"
#include "tempo_rte_logic.h"
#include "api_util.h"
#include "mqtt_ha_logic.h"
#include "mqtt_ha_events_logic.h"
#include "mqtt_ha_json_config_logic.h"
#include "balansun_source_health_logic.h"
#include "balansun_ha_state_payload.h"
#include "triac_api_shim.h"
#include "balansun_vacation_logic.h"
#include "storage_eeprom.h"
#include "balansun_persistence.h"
#include "metering/pmqtt_bindings.h"
#include <ArduinoJson.h>
#include "balansun_json.h"
#include <vector>
#include <balansun/platform/http_service.h>

void ApplyPmqttJsonPayload(const String &msg);
bool ApplyPmqttBindingsPayload(const String &topic, const String &msg, String *err);

static unsigned long previousPmqttIngestMillis = 0;
static unsigned long g_lastMqttConnectAttemptMs = 0;
static unsigned long g_lastMqttFailLogMs = 0;
static bool g_mqttHaLoggedConnected = false;

namespace {
constexpr uint32_t kMqttRoutineSocketTimeoutMs = 2500;
constexpr unsigned long kMqttConnectBackoffMs = 5000UL;
constexpr unsigned long kMqttFailLogIntervalMs = 30000UL;

void mqtt_pump_http(int passes = 4) { balansun_http_pump_server(server, passes); }
}  // namespace

/** Pmqtt source: subscribe to discovery broker for metering (independent of HA publish period). */
static bool pmqtt_ingest_mqtt_wanted() {
  if (balansun_active_source_get() != SourceId::Pmqtt) return false;
  const String host = ip32ToDotted(MQTTIP);
  if (host.length() == 0 || host == "0.0.0.0") return false;
  std::vector<String> topics;
  pmqtt_bindings_collect_topics(topics);
  if (!topics.empty()) return true;
  return PmqttTopic.length() > 0;
}

static void mqtt_apply_action_config_patch(int idx, const MqttActionConfigPatch &patch) {
  if (idx <= 0 || idx >= NbActions || load_channels[idx].periodCount == 0) return;
  bool changed = false;
  if (patch.threshold_w >= 0) {
    load_channels[idx].power_min[0] = patch.threshold_w;
    changed = true;
  }
  if (patch.power_min_w >= 0) {
    load_channels[idx].power_min[0] = patch.power_min_w;
    changed = true;
  }
  if (patch.power_max_w >= 0) {
    load_channels[idx].power_max[0] = patch.power_max_w;
    changed = true;
  }
  if (patch.hour_end >= 0) {
    load_channels[idx].period_end[0] = patch.hour_end;
    changed = true;
  }
  if (patch.mode_type > 0) {
    load_channels[idx].Type[0] = static_cast<byte>(patch.mode_type);
    changed = true;
  }
  if (changed) persistence_request_config_deferred();
}

static void subscribeMQTTCommands() {
  clientMQTT.subscribe((String(MQTTPrefix) + "/" + MQTTdeviceName + "/triac/set").c_str());
  clientMQTT.subscribe((String(MQTTPrefix) + "/" + MQTTdeviceName + "/source/set").c_str());
  clientMQTT.subscribe((String(MQTTPrefix) + "/" + MQTTdeviceName + "/vacation/set").c_str());
  clientMQTT.subscribe((String(MQTTPrefix) + "/" + MQTTdeviceName + "/max_routed_w/set").c_str());
  if (mqttJsonCommands) {
    clientMQTT.subscribe((String(MQTTPrefix) + "/" + MQTTdeviceName + "/site/config/set").c_str());
    clientMQTT.subscribe((String(MQTTPrefix) + "/" + MQTTdeviceName + "/vacation/config/set").c_str());
    for (int i = 1; i < NbActions; i++) {
      clientMQTT.subscribe(
          (String(MQTTPrefix) + "/" + MQTTdeviceName + "/action_" + String(i) + "/config/set").c_str());
    }
  }
  if (balansun_active_source_get() == SourceId::Pmqtt) {
    std::vector<String> pmqttTopics;
    pmqtt_bindings_collect_topics(pmqttTopics);
    if (!pmqttTopics.empty()) {
      for (const String &t : pmqttTopics) {
        clientMQTT.subscribe(t.c_str());
      }
    } else if (PmqttTopic.length() > 0) {
      clientMQTT.subscribe(PmqttTopic.c_str());
    }
  }
  for (int i = 1; i < NbActions; i++) {
    clientMQTT.subscribe((String(MQTTPrefix) + "/" + MQTTdeviceName + "/action_" + String(i) + "/set").c_str());
  }
}
static bool mqttEnsureConnected() {
  if (clientMQTT.connected()) return true;
  if (mqtt_publish_period_sec == 0 && !pmqtt_ingest_mqtt_wanted()) return false;
  const String host = ip32ToDotted(MQTTIP);
  if (host.length() == 0 || host == "0.0.0.0") return false;
  const unsigned long now = millis();
  if (g_lastMqttConnectAttemptMs != 0 && (now - g_lastMqttConnectAttemptMs) < kMqttConnectBackoffMs) {
    return false;
  }
  g_lastMqttConnectAttemptMs = now;
  MqttClient.setTimeout(kMqttRoutineSocketTimeoutMs);
  clientMQTT.setServer(host.c_str(), MQTTPort);
  clientMQTT.setCallback(callback);
  // ponytail: HA state JSON (~1 KB) and discovery configs need >256 B PubSubClient default.
  if (mqtt_publish_period_sec > 0 || pmqtt_ingest_mqtt_wanted()) {
    clientMQTT.setBufferSize(1536);
  }
  const String willTopic = mqttAvailabilityTopic();
  mqtt_pump_http();
  if (!clientMQTT.connect(MQTTdeviceName.c_str(), MQTTUser.c_str(), MQTTPwd.c_str(), willTopic.c_str(), 0, true,
                          "offline")) {
    if (now - g_lastMqttFailLogMs >= kMqttFailLogIntervalMs) {
      g_lastMqttFailLogMs = now;
      Serial.print("MQTT connection failed, error code= ");
      Serial.println(clientMQTT.state());
    }
    mqtt_pump_http();
    return false;
  }
  clientMQTT.publish(willTopic.c_str(), "online", true);
  subscribeMQTTCommands();
  mqtt_pump_http();
  return true;
}

void ApiMqttRepublishDiscovery() {
  mqtt_ha_discovered = false;
  if (mqttEnsureConnected()) {
    sendMQTTDiscoveryMsg_global();
    clientMQTT.loop();
  } else {
    previousMqttMillis = millis() - (unsigned long)(mqtt_publish_period_sec * 1000UL) - 1UL;
  }
}

void ApiMqttReconnect() {
  mqtt_ha_discovered = false;
  if (clientMQTT.connected()) {
    clientMQTT.publish(mqttAvailabilityTopic().c_str(), "offline", true);
    clientMQTT.disconnect();
  }
  previousMqttMillis = millis() - (unsigned long)(mqtt_publish_period_sec * 1000UL) - 1UL;
}

void ApiMqttPublishNow() {
  if (mqttEnsureConnected()) {
    if (!mqtt_ha_discovered) sendMQTTDiscoveryMsg_global();
    SendDataToHomeAssistant();
    clientMQTT.loop();
  } else {
    previousMqttMillis = millis() - (unsigned long)(mqtt_publish_period_sec * 1000UL) - 1UL;
  }
}

static const char *mqtt_connect_state_message(int state) {
  switch (state) {
    case -4:
      return "connection_timeout";
    case -3:
      return "connection_lost";
    case -2:
      return "connect_failed";
    case -1:
      return "disconnected";
    case 1:
      return "bad_protocol";
    case 2:
      return "bad_client_id";
    case 3:
      return "unavailable";
    case 4:
      return "bad_credentials";
    case 5:
      return "not_authorized";
    default:
      return "unknown";
  }
}

bool ApiMqttTestConnection(const String &host, uint16_t port, const String &user, const String &pwd,
                           const String &deviceName, int &errorCodeOut, String &messageOut) {
  errorCodeOut = 0;
  messageOut = "";
  if (host.length() == 0 || host == "0.0.0.0") {
    errorCodeOut = -1;
    messageOut = "invalid_broker_ip";
    return false;
  }
  if (deviceName.length() == 0) {
    errorCodeOut = -1;
    messageOut = "invalid_device_name";
    return false;
  }
  const bool wasConnected = clientMQTT.connected();
  if (wasConnected) {
    clientMQTT.publish(mqttAvailabilityTopic().c_str(), "offline", true);
    clientMQTT.disconnect();
  }
  MqttClient.setTimeout(8000);
  clientMQTT.setServer(host.c_str(), port);
  String willTopic = String(MQTTPrefix) + "/" + deviceName + "/availability";
  const bool ok =
      clientMQTT.connect(deviceName.c_str(), user.c_str(), pwd.c_str(), willTopic.c_str(), 0, true, "offline");
  if (ok) {
    clientMQTT.publish(willTopic.c_str(), "online", true);
    clientMQTT.disconnect();
    messageOut = "connected";
    errorCodeOut = 0;
  } else {
    errorCodeOut = clientMQTT.state();
    messageOut = mqtt_connect_state_message(errorCodeOut);
  }
  const String restoreHost = ip32ToDotted(MQTTIP);
  clientMQTT.setServer(restoreHost.c_str(), MQTTPort);
  if (wasConnected && mqtt_publish_period_sec > 0) {
    previousMqttMillis = millis() - (unsigned long)(mqtt_publish_period_sec * 1000UL) - 1UL;
  }
  return ok;
}

void publishMqttLoop() {  // Periodic MQTT: HA publish and/or Pmqtt ingest subscribe loop
  unsigned long tps = millis();
  const bool haPublish = mqtt_publish_period_sec > 0;
  const bool pmqttIngest = pmqtt_ingest_mqtt_wanted();

  if (pmqttIngest && !haPublish) {
    if (tps - previousPmqttIngestMillis < 1000) return;
    previousPmqttIngestMillis = tps;
    if (!mqttEnsureConnected()) return;
    clientMQTT.loop();
    return;
  }

  if (!haPublish) return;
  if (int((tps - previousMqttMillis) / 1000) <= (int)mqtt_publish_period_sec) return;
  previousMqttMillis = tps;
  if (!mqttEnsureConnected()) return;
  if (clientMQTT.connected()) {
    if (!g_mqttHaLoggedConnected) {
      g_mqttHaLoggedConnected = true;
      Serial.println(MQTTdeviceName + " connected");
      Debug.println(MQTTdeviceName + " connected");
    }
  } else {
    g_mqttHaLoggedConnected = false;
  }
  clientMQTT.loop();

  if (!mqtt_ha_discovered) {
    sendMQTTDiscoveryMsg_global();
  }
  SendDataToHomeAssistant();
  clientMQTT.loop();
  mqtt_pump_http();
}

void pmqtt_mqtt_service_tick(void) {
  if (!pmqtt_ingest_mqtt_wanted()) return;
  static unsigned long s_lastTickMs = 0;
  const unsigned long now = millis();
  if (s_lastTickMs != 0 && (now - s_lastTickMs) < 50UL) return;
  s_lastTickMs = now;

  /* HA state publish is driven by balansun_loop's 2s timer; avoid duplicate heavy work here. */
  if (mqtt_publish_period_sec > 0) {
    if (now - previousPmqttIngestMillis < 1000UL) return;
    previousPmqttIngestMillis = now;
    if (!clientMQTT.connected()) {
      mqttEnsureConnected();
      return;
    }
    for (int i = 0; i < 4; i++) {
      clientMQTT.loop();
      mqtt_pump_http(2);
      yield();
    }
    return;
  }

  publishMqttLoop();
  if (!clientMQTT.connected()) return;
  for (int i = 0; i < 4; i++) {
    clientMQTT.loop();
    mqtt_pump_http(2);
    yield();
  }
}

static void mqttPublishError(const String &failedTopic, const String &reason) {
  JsonDocument doc;
  doc["topic"] = failedTopic;
  doc["reason"] = reason;
  char buf[256];
  serializeJson(doc, buf, sizeof(buf));
  clientMQTT.publish(mqttErrorTopic().c_str(), buf);
}
// Callback: subscribed command topics → apply triac / source / actions
void callback(char *topic, byte *payload, unsigned int length) {
  String topicStr = String(topic);
  String msg = "";
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();
  if (balansun_active_source_get() == SourceId::Pmqtt) {
    pmqtt_bindings_cache_payload(topicStr, msg);
    if (PmqttBindingsJson.length() > 2) {
      String bindErr;
      if (ApplyPmqttBindingsPayload(topicStr, msg, &bindErr)) return;
    }
    if (PmqttTopic.length() > 0 && topicStr == PmqttTopic) {
      ApplyPmqttJsonPayload(msg);
      return;
    }
  }
  String err;
  String base = String(MQTTPrefix) + "/" + MQTTdeviceName + "/";
  if (!topicStr.startsWith(base)) return;
  String cmd = topicStr.substring(base.length());
  if (cmd == "triac/set") {
    balansun_apply_triac_command(msg.c_str(), err);
    return;
  }
  if (cmd == "source/set") {
    ApiSetSource(msg, true, err);
    mqtt_ha_discovered = false;
    return;
  }
  if (cmd == "vacation/set") {
    vacationEnabled = msg.equalsIgnoreCase("ON");
    if (!vacationEnabled) vacationEndEpoch = 0;
    persistence_request_config_deferred();
    return;
  }
  if (cmd == "max_routed_w/set") {
    maxRoutedW = static_cast<uint16_t>(msg.toInt());
    persistence_request_config_deferred();
    return;
  }
  if (mqttJsonCommands && cmd == "site/config/set") {
    const MqttSiteConfigPatch patch = mqtt_ha_json_config_parse_site(msg.c_str());
    if (!patch.ok) {
      mqttPublishError(topicStr,
                       patch.error.length() ? String(patch.error.c_str()) : String("invalid JSON"));
      return;
    }
    if (patch.max_routed_w >= 0) maxRoutedW = static_cast<uint16_t>(patch.max_routed_w);
    if (patch.has_triac_off_when_source_stale) triacOffWhenSourceStale = patch.triac_off_when_source_stale;
    if (patch.has_triac_backoff_when_heater_idle) triacBackoffWhenHeaterIdle = patch.triac_backoff_when_heater_idle;
    if (patch.action_daily_cap_index >= 0) {
      actionDailyCapWh[patch.action_daily_cap_index] = patch.action_daily_cap_wh;
    }
    persistence_request_config_deferred();
    return;
  }
  if (mqttJsonCommands && cmd == "vacation/config/set") {
    const MqttVacationConfigPatch patch = mqtt_ha_json_config_parse_vacation(msg.c_str());
    if (!patch.ok) {
      mqttPublishError(topicStr,
                       patch.error.length() ? String(patch.error.c_str()) : String("invalid JSON"));
      return;
    }
    if (patch.has_vacation_enabled) vacationEnabled = patch.vacation_enabled;
    if (patch.vacation_end_epoch >= 0) vacationEndEpoch = static_cast<uint32_t>(patch.vacation_end_epoch);
    persistence_request_config_deferred();
    return;
  }
  if (mqttJsonCommands && cmd.indexOf("/config/set") > 0 && cmd.startsWith("action_")) {
    String idxStr = cmd.substring(7, cmd.indexOf("/config"));
    int idx = idxStr.toInt();
    const MqttActionConfigPatch patch = mqtt_ha_json_config_parse_action(msg.c_str());
    if (!patch.ok) {
      mqttPublishError(topicStr,
                       patch.error.length() ? String(patch.error.c_str()) : String("invalid JSON"));
      return;
    }
    mqtt_apply_action_config_patch(idx, patch);
    return;
  }
  if (cmd.startsWith("action_") && cmd.endsWith("/set")) {
    String idxStr = cmd.substring(7, cmd.length() - 4);
    int idx = idxStr.toInt();
    MqttActionCmdKind actionCmd;
    if (mqtt_ha_command_parse_action(msg.c_str(), &actionCmd)) {
      if (actionCmd == MqttActionCmdKind::On) {
        ApiSetActionOverride(idx, "on", 0, 0, err);
      } else if (actionCmd == MqttActionCmdKind::Off) {
        ApiSetActionOverride(idx, "off", 0, 0, err);
      } else if (actionCmd == MqttActionCmdKind::Auto) {
        ApiSetActionOverride(idx, "auto", 0, 0, err);
      }
    }
  }
}
//*************************************************************************
//*          CONFIG OF DISCOVERY MESSAGE FOR HOME ASSISTANT  / DOMOTICZ             *
//*************************************************************************

static void mqttPublishHaEvent(const char *type) {
  JsonDocument ev;
  ev["type"] = type;
  char buf[128];
  serializeJson(ev, buf, sizeof(buf));
  clientMQTT.publish(mqttEventTopic().c_str(), buf);
}

static void mqttPublishHaEventsFromState(bool surplus_active, bool source_stale, const char *ltarf) {
  static bool prev_surplus = false;
  static bool prev_stale = false;
  static bool prev_cap = false;
  static bool prev_hunting = false;
  static bool prev_vacation = false;
  static bool prev_action_cap = false;
  static String prev_ltarf;
  const uint32_t now_epoch = static_cast<uint32_t>(time(NULL));
  const bool vacation_active =
      balansun_vacation_logic_active(vacationEnabled, vacationEndEpoch, now_epoch);
  bool any_action_cap = false;
  for (int i = 0; i < NbActions; i++) {
    if (actionCapHit[i]) {
      any_action_cap = true;
      break;
    }
  }
  MqttHaEventInput in;
  in.surplus_active = surplus_active;
  in.prev_surplus_active = prev_surplus;
  in.source_stale = source_stale;
  in.prev_source_stale = prev_stale;
  in.site_cap_active = siteCapActive;
  in.prev_site_cap_active = prev_cap;
  in.regulation_hunting = g_regulation_hunting_active;
  in.prev_regulation_hunting = prev_hunting;
  in.vacation_active = vacation_active;
  in.prev_vacation_active = prev_vacation;
  in.action_cap_hit = any_action_cap;
  in.prev_action_cap_hit = prev_action_cap;
  in.linky_tariff = ltarf ? ltarf : "";
  in.prev_linky_tariff = prev_ltarf.c_str();
  MqttHaEventOutput out;
  mqtt_ha_events_logic_detect(in, &out);
  if (out.surplus_started) mqttPublishHaEvent("surplus_started");
  if (out.surplus_ended) mqttPublishHaEvent("surplus_ended");
  if (out.source_lost) mqttPublishHaEvent("source_lost");
  if (out.triac_cap_hit) mqttPublishHaEvent("triac_cap_hit");
  if (out.linky_tariff_changed) mqttPublishHaEvent("linky_tariff_changed");
  if (out.regulation_hunting_started) mqttPublishHaEvent("regulation_hunting");
  if (out.vacation_ended) mqttPublishHaEvent("vacation_ended");
  if (out.action_cap_hit) mqttPublishHaEvent("action_cap_hit");
  prev_surplus = surplus_active;
  prev_stale = source_stale;
  prev_cap = siteCapActive;
  prev_hunting = g_regulation_hunting_active;
  prev_vacation = vacation_active;
  prev_action_cap = any_action_cap;
  if (ltarf) prev_ltarf = String(ltarf);
}
//****************************************
//* PUBLISH STATE PAYLOAD TO HOME ASSISTANT *
//****************************************

void SendDataToHomeAssistant() {
  String StateTopic = mqttStateTopic();
  BalansunJsonDoc _balansunJsonPool1 = balansun_json_doc_alloc(1536);
  JsonDocument &doc = _balansunJsonPool1;
  char buffer[1536];
  balansun_append_mqtt_ha_state_payload(doc.to<JsonObject>());
  const int health = balansun_compute_source_health_score();
  const int triac_open = TriacGetOpenPercent();
  size_t n = serializeJson(doc, buffer);
  bool published = clientMQTT.publish(StateTopic.c_str(), buffer, n);
  clientMQTT.publish(mqttAvailabilityTopic().c_str(), "online", true);
  mqttPublishHaEventsFromState(triac_open > 5, balansun_source_health_is_stale(health),
                               balansun_cap_mqtt_linky_tariff() ? LTARF.c_str() : "");
  doc.clear();
  buffer[0] = '\0';



}  // END SendDataToHomeAssistant
