/*
 * Auto-split from api_v1_routes.cpp — see api_v1_common.h
 */
#include "api_v1_common.h"
#include "balansun_ha_state_payload.h"
#include "balansun_regulation_logic.h"
#include "balansun_regulation_state.h"
#include "balansun_config_daily_cap_logic.h"
#include "balansun_diag.h"
#include "balansun_triac_calibration_logic.h"
#include "metering/pmqtt_bindings.h"
#include "balansun_temperature.h"
#include "balansun_product_caps.h"
#include "balansun_config_cap_logic.h"
#include "balansun_self_test_safety_runtime.h"
#include "balansun_triac_isr.h"
#include "balansun_triac_sync_logic.h"
#include <balansun/load_profile.h>

namespace {

void balansun_trim_inplace(char *s) {
  if (!s || !s[0]) return;
  char *start = s;
  while (*start && (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n')) start++;
  if (start != s) memmove(s, start, strlen(start) + 1);
  const size_t len = strlen(s);
  size_t end = len;
  while (end > 0 && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) {
    end--;
  }
  s[end] = '\0';
}

void balansun_tolower_inplace(char *s) {
  for (; s && *s; ++s) {
    if (*s >= 'A' && *s <= 'Z') *s = static_cast<char>(*s - 'A' + 'a');
  }
}

void balansun_normalize_pmqtt_path_inplace(char *path) {
  balansun_trim_inplace(path);
  if (strcmp(path, "$") == 0) {
    path[0] = '\0';
    return;
  }
  if (strncmp(path, "$.", 2) == 0) memmove(path, path + 2, strlen(path + 2) + 1);
  else if (path[0] == '$') memmove(path, path + 1, strlen(path + 1) + 1);
  while (path[0] == '.') memmove(path, path + 1, strlen(path + 1) + 1);
}

}  // namespace

void api_append_config_object(JsonObject o) {
  o["dhcp_on"] = (bool)(dhcpOn == 1);
  o["ip_fixed"] = ip32ToDotted(IP_Fixe);
  o["gateway"] = ip32ToDotted(Gateway);
  o["subnet_mask"] = ip32ToDotted(subnetMask);
  o["dns"] = ip32ToDotted(dns);
  o["source"] = Source;
  o["ext_peer_ip"] = ip32ToDotted(ext_peer_ip);
  o["ext_peer_port"] = ext_peer_port;
  o["ext_peer_path"] = ext_peer_path;
  o["ext_protocol"] = ext_peer_protocol.length() ? ext_peer_protocol : "json";
  o["enphase_user"] = EnphaseUser;
  o["enphase_password"] = EnphasePwd;
  o["meter_channel"] = meter_channel;
  o["enphase_serial"] = meter_channel;
  o["mqtt_repeat_sec"] = mqtt_publish_period_sec;
  o["mqtt_ip"] = ip32ToDotted(MQTTIP);
  o["mqtt_port"] = MQTTPort;
  o["mqtt_user"] = MQTTUser;
  o["mqtt_password"] = MQTTPwd;
  o["mqtt_prefix"] = MQTTPrefix;
  o["mqtt_device_name"] = MQTTdeviceName;
  o["router_name"] = routerName;
  o["probe_second_name"] = probeSecondName;
  o["probe_house_name"] = probeHouseName;
  o["temperature_label"] = temperatureSensorName;
  balansun_temperature_append_config_json(o);
  o["calib_u"] = CalibU;
  o["calib_i"] = CalibI;
  o["pmqtt_topic"] = PmqttTopic;
  o["pmqtt_schema"] = PmqttSchema;
  o["jsy_mk333_serial_baud"] = JsyMk333SerialBaud;
  o["install_country"] = balansun_mains_install_country();
  o["install_country_variant"] = balansun_mains_install_variant();
  o["mains_nominal_v"] = balansun_mains_nominal_v();
  o["mains_frequency_mode"] =
      (balansun_mains_frequency_mode() == MainsFrequencyMode::Manual) ? "manual" : "auto";
  o["mains_frequency_hz_manual"] = balansun_mains_frequency_hz_manual();
  o["mains_frequency_effective_hz"] = balansun_mains_effective_frequency_hz();
  o["mains_frequency_source"] = balansun_mains_frequency_source_string();
  const char *warn = balansun_mains_frequency_warning_string();
  if (warn) {
    o["mains_frequency_warning"] = warn;
  } else {
    o["mains_frequency_warning"] = nullptr;
  }
  o["triac_override_max_temp_c"] = triacOverrideMaxTempC;
  o["http_cors_enabled"] = httpCorsEnabled;
  o["pwm_gpio"] = pwmGpio;
  o["pwm_mode"] = pwmMode.length() ? pwmMode : "off";
  o["pwm_duty_percent"] = pwmDutyPercent;
  o["pwm_inverted"] = pwmInverted;
  o["status_led_mode"] = balansun_status_led_mode_string(static_cast<StatusLedMode>(statusLedMode));
  o["status_led_gpio_activity"] = statusLedGpioActivity;
  o["status_led_gpio_regulation"] = statusLedGpioRegulation;
  o["status_led_rgb_gpio"] = statusLedRgbGpio;
  o["status_led_active_low"] = statusLedActiveLow;
  JsonArray actColor = o["status_led_color_activity"].to<JsonArray>();
  actColor.add(statusLedColorActivity.r);
  actColor.add(statusLedColorActivity.g);
  actColor.add(statusLedColorActivity.b);
  JsonArray regColor = o["status_led_color_regulation"].to<JsonArray>();
  regColor.add(statusLedColorRegulation.r);
  regColor.add(statusLedColorRegulation.g);
  regColor.add(statusLedColorRegulation.b);
  JsonArray rebootColor = o["status_led_color_reboot"].to<JsonArray>();
  rebootColor.add(statusLedColorReboot.r);
  rebootColor.add(statusLedColorReboot.g);
  rebootColor.add(statusLedColorReboot.b);
  JsonArray apColor = o["status_led_color_ap"].to<JsonArray>();
  apColor.add(statusLedColorAp.r);
  apColor.add(statusLedColorAp.g);
  apColor.add(statusLedColorAp.b);
  o["tempo_rte_enabled"] = tempoRteEnabled;
  o["expert_regulation_mode"] = expert_regulation_mode;
  o["regulation_gain"] = regulation_gain;
  o["triac_cal_enabled"] = g_triac_cal.enabled;
  JsonArray cal = o["triac_calibration"].to<JsonArray>();
  for (int i = 0; i < 3; i++) {
    JsonObject p = cal.add<JsonObject>();
    p["duty_pct"] = g_triac_cal.points[i].duty_pct;
    p["measured_w"] = g_triac_cal.points[i].measured_w;
  }
  o["hunting_reversal_threshold"] = g_regulation_hunting_config.reversal_threshold;
  o["hunting_window_min"] = g_regulation_hunting_config.window_min;
  o["vacation_enabled"] = vacationEnabled;
  o["vacation_end_epoch"] = vacationEndEpoch;
  o["max_routed_w"] = maxRoutedW;
  o["mqtt_json_commands"] = mqttJsonCommands;
  o["triac_off_when_source_stale"] = triacOffWhenSourceStale;
  o["commissioning_blocks_outputs"] = commissioningBlocksOutputs;
  o["triac_backoff_when_heater_idle"] = triacBackoffWhenHeaterIdle;
  o["load_profile"] = loadProfileWire.c_str();
  JsonArray caps = o["action_daily_cap_wh"].to<JsonArray>();
  for (int i = 0; i < NbActions; i++) {
    caps.add(actionDailyCapWh[i]);
  }
}

static bool config_validate_caps(JsonObject root, String &err) {
  if (balansun_api_safety_lockout_active()) {
    for (JsonPair kv : root) {
      if (balansun_config_key_blocked_by_safety_lockout(kv.key().c_str())) {
        err = "safety_lockout";
        return false;
      }
    }
  }
  const BalansunConfigCapContext ctx = balansun_config_cap_context_default();
  for (JsonPair kv : root) {
    char missing[64] = "";
    const char *value = nullptr;
    char valueBuf[64];
    const char *key = kv.key().c_str();
    if (strcmp(key, "source") == 0 && !kv.value().isNull()) {
      String s = kv.value().as<String>();
      s.toCharArray(valueBuf, sizeof(valueBuf));
      value = valueBuf;
    } else if (strcmp(key, "pwm_mode") == 0 && !kv.value().isNull()) {
      const char *ms = kv.value().as<const char *>();
      value = ms;
    }
    if (!balansun_config_key_allowed(ctx, key, value, missing)) {
      err = String("cap:") + missing;
      return false;
    }
  }
  return true;
}

bool config_apply_from_json(JsonObject root, bool fullPut, String &err) {
  if (!config_validate_caps(root, err)) return false;
  const char *keys[] = {"dhcp_on", "ip_fixed", "gateway", "subnet_mask", "dns", "source", "ext_peer_ip",
                       "ext_peer_port", "ext_peer_path", "ext_protocol", "enphase_user", "enphase_password",
                       "meter_channel", "enphase_serial",
                       "mqtt_repeat_sec", "mqtt_ip", "mqtt_port", "mqtt_user", "mqtt_password", "mqtt_prefix",
                       "mqtt_device_name", "router_name", "probe_second_name", "probe_house_name",
                       "temperature_label", "calib_u", "calib_i", "pmqtt_topic", "pmqtt_schema",
                       "jsy_mk333_serial_baud",
                       "install_country", "install_country_variant", "mains_nominal_v", "mains_frequency_mode",
                       "mains_frequency_hz_manual", "triac_override_max_temp_c", "http_cors_enabled"};
  const int nkeys = sizeof(keys) / sizeof(keys[0]);
  if (fullPut) {
    for (int i = 0; i < nkeys; i++) {
      if (root[keys[i]].isNull()) {
        err = String("missing key ") + keys[i];
        return false;
      }
    }
  }
  auto applyIp = [&](const char *k, unsigned long &dst) -> bool {
    if (root[k].isNull()) return true;
    uint32_t v;
    const char *s = root[k].as<const char *>();
    if (!s || !dottedToIp32(s, v)) {
      err = String("bad ip for ") + k;
      return false;
    }
    dst = v;
    return true;
  };
  if (!root["dhcp_on"].isNull()) dhcpOn = root["dhcp_on"].as<bool>() ? 1 : 0;
  if (!applyIp("ip_fixed", IP_Fixe)) return false;
  if (!applyIp("gateway", Gateway)) return false;
  if (!applyIp("subnet_mask", subnetMask)) return false;
  if (!applyIp("dns", dns)) return false;
  if (!root["source"].isNull()) {
    String next = root["source"].as<String>();
    if (!balansun_source_wire_supported(next)) {
      err = "cap:unsupported_source";
      return false;
    }
    Source = next;
    balansun_active_source_refresh_from_global_string();
  }
  if (!root["ext_peer_ip"].isNull()) {
    if (!applyIp("ext_peer_ip", ext_peer_ip)) return false;
  }
  if (!root["ext_peer_port"].isNull()) {
    int p = (int)root["ext_peer_port"];
    if (p < 1 || p > 65535) {
      err = "ext_peer_port must be 1..65535";
      return false;
    }
    ext_peer_port = (unsigned int)p;
  }
  if (!root["ext_peer_path"].isNull()) {
    String path = root["ext_peer_path"].as<String>();
    path.trim();
    if (path.length() == 0 || path[0] != '/') {
      err = "ext_peer_path must be non-empty and start with /";
      return false;
    }
    if (path.indexOf("..") >= 0) {
      err = "ext_peer_path invalid";
      return false;
    }
    if (path.length() > 48) {
      err = "ext_peer_path too long (max 48)";
      return false;
    }
    ext_peer_path = path;
  }
  if (!root["ext_protocol"].isNull()) {
    String proto = root["ext_protocol"].as<String>();
    proto.trim();
    proto.toLowerCase();
    if (proto != "json") {
      err = "ext_protocol must be json";
      return false;
    }
    ext_peer_protocol = "json";
  }
  if (!root["enphase_user"].isNull()) EnphaseUser = root["enphase_user"].as<String>();
  if (!root["enphase_password"].isNull()) EnphasePwd = root["enphase_password"].as<String>();
  if (!root["meter_channel"].isNull()) meter_channel = root["meter_channel"].as<String>();
  else if (!root["enphase_serial"].isNull()) meter_channel = root["enphase_serial"].as<String>();
  if (!root["mqtt_repeat_sec"].isNull()) mqtt_publish_period_sec = (unsigned int)(int)root["mqtt_repeat_sec"];
  if (!applyIp("mqtt_ip", MQTTIP)) return false;
  if (!root["mqtt_port"].isNull()) MQTTPort = (unsigned int)(int)root["mqtt_port"];
  if (!root["mqtt_user"].isNull()) MQTTUser = root["mqtt_user"].as<String>();
  if (!root["mqtt_password"].isNull()) MQTTPwd = root["mqtt_password"].as<String>();
  if (!root["mqtt_prefix"].isNull()) MQTTPrefix = root["mqtt_prefix"].as<String>();
  if (!root["mqtt_device_name"].isNull()) {
    MQTTdeviceName = root["mqtt_device_name"].as<String>();
    balansun_apply_default_mqtt_device_name(MQTTdeviceName);
  }
  if (!root["router_name"].isNull()) routerName = root["router_name"].as<String>();
  if (!root["probe_second_name"].isNull()) probeSecondName = root["probe_second_name"].as<String>();
  if (!root["probe_house_name"].isNull()) probeHouseName = root["probe_house_name"].as<String>();
  if (!root["temp_gpio"].isNull() || !root["temperature_slots"].isNull() || !root["temperature_label"].isNull()) {
    if (!balansun_temperature_apply_config_json(root, !root["temperature_slots"].isNull(),
                                             !root["temp_gpio"].isNull(), !root["temperature_label"].isNull(),
                                             err)) {
      return false;
    }
  }
  if (!root["calib_u"].isNull()) CalibU = (unsigned int)(int)root["calib_u"];
  if (!root["calib_i"].isNull()) CalibI = (unsigned int)(int)root["calib_i"];
  bool pmqttBindingsChecked = false;
  if (!root["pmqtt_topic"].isNull()) PmqttTopic = root["pmqtt_topic"].as<String>();
  if (!root["pmqtt_schema"].isNull()) PmqttSchema = root["pmqtt_schema"].as<String>();
  if (!root["pmqtt_bindings"].isNull()) {
    if (!root["pmqtt_bindings"].is<JsonArray>()) {
      err = "pmqtt_bindings must be an array";
      return false;
    }
    JsonArray arr = root["pmqtt_bindings"].as<JsonArray>();
    if (arr.size() == 0 && Source == String("Pmqtt")) {
      err = "pmqtt_bindings required for Pmqtt source";
      return false;
    }
    if (arr.size() > 24) {
      err = "pmqtt_bindings max 24";
      return false;
    }
    {
      const BalansunHwCaps caps = balansun_hw_caps_now();
      BalansunJsonDoc _balansunJsonPool1 = balansun_json_doc_alloc(caps.pmqtt_bindings_apply_cap);
  JsonDocument &bindsDoc = _balansunJsonPool1;
      JsonArray out = bindsDoc.to<JsonArray>();
      for (JsonVariantConst v : arr) {
        if (!v.is<JsonObjectConst>()) {
          err = "pmqtt_bindings items must be objects";
          return false;
        }
        JsonObjectConst inObj = v.as<JsonObjectConst>();
        const char *metric = inObj["metric"] | "";
        const char *topic = inObj["topic"] | "";
        const char *format = inObj["format"] | "";
        const char *path = inObj["path"] | "";
        const bool enabled = !inObj["enabled"].isNull() ? inObj["enabled"].as<bool>() : true;
        char metricBuf[64];
        char topicBuf[129];
        char formatBuf[16];
        char pathBuf[128];
        strncpy(metricBuf, metric, sizeof(metricBuf) - 1);
        metricBuf[sizeof(metricBuf) - 1] = '\0';
        strncpy(topicBuf, topic, sizeof(topicBuf) - 1);
        topicBuf[sizeof(topicBuf) - 1] = '\0';
        strncpy(formatBuf, format, sizeof(formatBuf) - 1);
        formatBuf[sizeof(formatBuf) - 1] = '\0';
        strncpy(pathBuf, path, sizeof(pathBuf) - 1);
        pathBuf[sizeof(pathBuf) - 1] = '\0';
        balansun_trim_inplace(metricBuf);
        balansun_trim_inplace(topicBuf);
        balansun_trim_inplace(formatBuf);
        balansun_tolower_inplace(formatBuf);
        balansun_normalize_pmqtt_path_inplace(pathBuf);
        if (metricBuf[0] == '\0') {
          err = "pmqtt_bindings.metric required";
          return false;
        }
        if (topicBuf[0] == '\0') {
          strncpy(topicBuf, PmqttTopic.c_str(), sizeof(topicBuf) - 1);
          topicBuf[sizeof(topicBuf) - 1] = '\0';
        }
        if (topicBuf[0] == '\0') {
          err = "pmqtt_bindings.topic required";
          return false;
        }
        if (strlen(topicBuf) > 128) {
          err = "pmqtt_bindings.topic too long";
          return false;
        }
        if (formatBuf[0] == '\0') {
          strncpy(formatBuf, "json", sizeof(formatBuf) - 1);
          formatBuf[sizeof(formatBuf) - 1] = '\0';
        }
        if (strcmp(formatBuf, "plain") != 0 && strcmp(formatBuf, "json") != 0 &&
            strcmp(formatBuf, "snapshot") != 0) {
          err = "pmqtt_bindings.format must be plain/json/snapshot";
          return false;
        }
        JsonObject o = out.add<JsonObject>();
        o["metric"] = metricBuf;
        o["topic"] = topicBuf;
        o["format"] = formatBuf;
        if (pathBuf[0] != '\0') o["path"] = pathBuf;
        o["enabled"] = enabled;
      }
      if (!pmqtt_bindings_array_has_required_power(out)) {
        err = "pmqtt_missing_required_power";
        return false;
      }
      pmqttBindingsChecked = true;
      String ser;
      serializeJson(out, ser);
      PmqttBindingsJson = ser;
      pmqtt_bindings_invalidate_cache();
    }
  }
  if (!root["jsy_mk333_serial_baud"].isNull()) JsyMk333SerialBaud = (uint32_t)root["jsy_mk333_serial_baud"].as<unsigned long>();
  if (!root["install_country"].isNull() || !root["install_country_variant"].isNull()) {
    String cc = !root["install_country"].isNull() ? root["install_country"].as<String>()
                                                    : String(balansun_mains_install_country());
    String var = !root["install_country_variant"].isNull()
                     ? root["install_country_variant"].as<String>()
                     : String(balansun_mains_install_variant());
    cc.trim();
    cc.toUpperCase();
    if (cc.length() > 2) cc = cc.substring(0, 2);
    var.trim();
    char countryBuf[4] = "FR";
    char variantBuf[12] = "";
    cc.toCharArray(countryBuf, sizeof(countryBuf));
    var.toCharArray(variantBuf, sizeof(variantBuf));
    balansun_mains_country_apply(countryBuf, variantBuf[0] ? variantBuf : nullptr);
  }
  if (!root["mains_frequency_mode"].isNull()) {
    const char *fm = root["mains_frequency_mode"];
    balansun_mains_set_frequency_mode((fm && strcmp(fm, "manual") == 0) ? MainsFrequencyMode::Manual
                                                                     : MainsFrequencyMode::Auto);
  }
  if (!root["mains_frequency_hz_manual"].isNull()) {
    int hz = (int)root["mains_frequency_hz_manual"];
    balansun_mains_set_frequency_hz_manual((uint8_t)((hz == 60) ? 60 : 50));
  }
  if (!root["triac_override_max_temp_c"].isNull()) {
    int cap = (int)root["triac_override_max_temp_c"];
    triacOverrideMaxTempC = (cap < 0) ? 0 : (cap > 120 ? 120 : cap);
  }
  if (!root["http_cors_enabled"].isNull()) {
    httpCorsEnabled = root["http_cors_enabled"].as<bool>();
  }
  if (!root["mains_nominal_v"].isNull()) {
    balansun_mains_nominal_v_set((uint16_t)(int)root["mains_nominal_v"]);
  }
  if (balansun_mains_frequency_mode() == MainsFrequencyMode::Manual) {
    balansun_mains_apply_effective_hz(balansun_mains_frequency_hz_manual(), MainsFrequencySource::Manual);
  } else if (mains_frequency_hz >= 45.0f && mains_frequency_hz <= 65.0f) {
    balansun_mains_on_meter_frequency(mains_frequency_hz);
  }
  if (balansun_active_source_get() != SourceId::BalansunPeer) {
    Source_data = Source;
  }
  if (!root["pwm_gpio"].isNull()) {
    int g = (int)root["pwm_gpio"];
    std::string errStd;
    if (!balansun_pwm_logic_validate_gpio(g, errStd)) {
      err = String(errStd.c_str());
      return false;
    }
    pwmGpio = g;
  }
  if (!root["pwm_mode"].isNull()) {
    PwmMode mode;
    const char *ms = root["pwm_mode"].as<const char *>();
    std::string errStd;
    if (!balansun_pwm_logic_parse_mode(ms, mode, errStd)) {
      err = String(errStd.c_str());
      return false;
    }
    pwmMode = String(ms);
  }
  if (!root["pwm_duty_percent"].isNull()) {
    int d = (int)root["pwm_duty_percent"];
    if (d < 0 || d > 100) {
      err = "pwm_duty_percent must be 0..100";
      return false;
    }
    pwmDutyPercent = d;
  }
  if (!root["pwm_inverted"].isNull()) pwmInverted = root["pwm_inverted"].as<bool>();
  if (!root["tempo_rte_enabled"].isNull()) {
    const bool next = root["tempo_rte_enabled"].as<bool>();
    if (next && !tempoRteEnabled) {
      tempoRteLastPollDecihours = -1;
      tempoRteEnabled = next;
      tempo_rte_poll();
    } else {
      tempoRteEnabled = next;
    }
  }
  if (!root["expert_regulation_mode"].isNull()) {
    int m = (int)root["expert_regulation_mode"];
    expert_regulation_mode = (m > 0) ? 1 : 0;
  }
  if (!root["regulation_gain"].isNull()) {
    int r = (int)root["regulation_gain"];
    if (r < 1) r = 1;
    if (r > 99) r = 99;
    regulation_gain = static_cast<uint8_t>(r);
  }
  if (!root["triac_cal_enabled"].isNull()) {
    g_triac_cal.enabled = root["triac_cal_enabled"].as<bool>();
  }
  if (!root["triac_calibration"].isNull() && root["triac_calibration"].is<JsonArray>()) {
    JsonArray arr = root["triac_calibration"].as<JsonArray>();
    for (size_t i = 0; i < 3 && i < arr.size(); i++) {
      JsonObject p = arr[i];
      g_triac_cal.points[i].duty_pct = (uint8_t)(int)p["duty_pct"];
      g_triac_cal.points[i].measured_w = (uint16_t)(int)p["measured_w"];
    }
  }
  if (!root["hunting_reversal_threshold"].isNull()) {
    int t = (int)root["hunting_reversal_threshold"];
    if (t >= 3 && t <= 30) g_regulation_hunting_config.reversal_threshold = (uint8_t)t;
  }
  if (!root["hunting_window_min"].isNull()) {
    int w = (int)root["hunting_window_min"];
    if (w >= 2 && w <= 30) g_regulation_hunting_config.window_min = (uint16_t)w;
  }
  if (!root["vacation_enabled"].isNull()) vacationEnabled = root["vacation_enabled"].as<bool>();
  if (!root["vacation_end_epoch"].isNull()) {
    vacationEndEpoch = (uint32_t)root["vacation_end_epoch"].as<unsigned long>();
  }
  if (!root["max_routed_w"].isNull()) {
    int mw = (int)root["max_routed_w"];
    if (mw < 0) mw = 0;
    if (mw > 65535) mw = 65535;
    maxRoutedW = static_cast<uint16_t>(mw);
  }
  if (!root["mqtt_json_commands"].isNull()) mqttJsonCommands = root["mqtt_json_commands"].as<bool>();
  if (!root["triac_off_when_source_stale"].isNull()) {
    triacOffWhenSourceStale = root["triac_off_when_source_stale"].as<bool>();
  }
  if (!root["commissioning_blocks_outputs"].isNull()) {
    commissioningBlocksOutputs = root["commissioning_blocks_outputs"].as<bool>();
  }
  if (!root["triac_backoff_when_heater_idle"].isNull()) {
    triacBackoffWhenHeaterIdle = root["triac_backoff_when_heater_idle"].as<bool>();
  }
  if (!root["load_profile"].isNull()) {
    const char *lp = root["load_profile"].as<const char *>();
    if (lp && lp[0]) {
      loadProfileWire.assign(balansun_load_profile_wire(balansun_load_profile_from_wire(lp)));
    }
  }
  if (!root["action_daily_cap_wh"].isNull() && root["action_daily_cap_wh"].is<JsonArray>()) {
    JsonArray arr = root["action_daily_cap_wh"].as<JsonArray>();
    uint32_t patch[20];
    const size_t n = arr.size() < 20u ? arr.size() : 20u;
    for (size_t i = 0; i < n; i++) {
      patch[i] = (uint32_t)arr[i].as<unsigned long>();
    }
    balansun_config_daily_cap_apply(actionDailyCapWh, NbActions, patch, n);
  }
  if (Source == "Pmqtt" && PmqttBindingsJson.length() > 2 && !pmqttBindingsChecked) {
    String missing;
    if (!pmqtt_bindings_has_required_power(&missing)) {
      err = "pmqtt_missing_required_power";
      return false;
    }
  }
  if (!balansun_status_led_apply_config_json(root, err)) return false;
  return true;
}

bool ApiSetSource(const String &nextSource, bool persist, String &err) {
  if (!balansun_source_wire_supported(nextSource)) {
    err = "unsupported source";
    return false;
  }
  Source = nextSource;
  balansun_active_source_refresh_from_global_string();
  if (balansun_active_source_get() != SourceId::BalansunPeer) Source_data = Source;
  if (persist) persistConfigToEeprom();
  ApiMqttReconnect();
  return true;
}

const char *override_state_name(byte state) {
  if (state == ACTION_OVERRIDE_ON) return "on";
  if (state == ACTION_OVERRIDE_OFF) return "off";
  if (state == ACTION_OVERRIDE_TRIAC_FIXED) return "triac_fixed";
  return "auto";
}

byte override_state_from_name(const char *state) {
  if (!state) return 255;
  if (strcasecmp(state, "auto") == 0) return ACTION_OVERRIDE_AUTO;
  if (strcasecmp(state, "on") == 0) return ACTION_OVERRIDE_ON;
  if (strcasecmp(state, "off") == 0) return ACTION_OVERRIDE_OFF;
  if (strcasecmp(state, "triac_fixed") == 0) return ACTION_OVERRIDE_TRIAC_FIXED;
  return 255;
}

static void api_sync_action0_override_triac(byte override_state, int triac_percent) {
  TriacRegulationInput tin;
  tin.override_state = override_state;
  tin.override_triac_percent = triac_percent;
  tin.actif = load_channels[0].Actif;
  tin.schedule_type_triac = 0;
  tin.net_power_w = 0;
  tin.current_triac_delay_percent_f = g_triac_delay_percent_f[0];
  const TriacRegulationOutput tout = balansun_regulation_compute_triac(tin);
  g_triac_delay_percent_f[0] = tout.triac_delay_percent_f;
  g_triac_delay_percent[0] = static_cast<int>(tout.triac_delay_percent_f + 0.5f);
  balansun_regulation_sync_triac_globals();
}

bool ApiSetActionOverride(int idx, const char *state, int triacPercent, unsigned long durationSec, String &err) {
  const ActionOverrideValidation v = actions_api_logic_validate_override(
      idx, kMaxRoutingActions, state, triacPercent, temperature, triacOverrideMaxTempC);
  if (!v.ok) {
    err = String(v.error.c_str());
    return false;
  }
  const byte st = v.override_state;
  if (st == ACTION_OVERRIDE_AUTO) {
    load_channels[idx].ClearOverride();
  } else {
    load_channels[idx].SetOverride(st, (byte)triacPercent, durationSec);
  }
  if (idx == 0 && st != ACTION_OVERRIDE_AUTO) {
    api_sync_action0_override_triac(st, triacPercent);
  }
  return true;
}

void api_append_action_override(JsonObject o, int idx) {
  if (idx < 0 || idx >= kMaxRoutingActions) return;
  if (load_channels[idx].OverrideExpired(millis())) load_channels[idx].ClearOverride();
  o["index"] = idx;
  o["state"] = override_state_name(load_channels[idx].OverrideState);
  o["triac_open_percent"] = load_channels[idx].OverrideTriacPercent;
  o["sticky"] = load_channels[idx].OverrideState != ACTION_OVERRIDE_AUTO && load_channels[idx].OverrideUntilMillis == 0;
  if (load_channels[idx].OverrideState != ACTION_OVERRIDE_AUTO && load_channels[idx].OverrideUntilMillis != 0) {
    long msLeft = (long)(load_channels[idx].OverrideUntilMillis - millis());
    o["expires_in_s"] = msLeft > 0 ? (int)(msLeft / 1000) : 0;
  } else {
    o["expires_in_s"] = 0;
  }
}

void api_append_measurements_object(JsonObject doc) {
  BalansunPublic r = BalansunReadSnapshot();
  doc["date_valid"] = time_sync_valid;
  doc["date"] = time_sync_valid ? sync_clock_str : "Waiting for time sync (Internet)";
  doc["source"] = Source_data;
  doc["probe_house_name"] = probeHouseName;
  doc["probe_second_name"] = probeSecondName;
  doc["house"]["active_import_w"] = r.house_active_import_w;
  doc["house"]["active_export_w"] = r.house_active_export_w;
  doc["house"]["apparent_import_va"] = r.house_apparent_import_va;
  doc["house"]["apparent_export_va"] = r.house_apparent_export_va;
  doc["house"]["energy_day_import_wh"] = r.house_day_energy_import_wh;
  doc["house"]["energy_day_export_wh"] = r.house_day_energy_export_wh;
  doc["house"]["energy_total_import_wh"] = r.house_energy_import_wh;
  doc["house"]["energy_total_export_wh"] = r.house_energy_export_wh;
  {
    const int grid_net_w = r.house_active_import_w - r.house_active_export_w;
    doc["house"]["grid_net_w"] = grid_net_w;
    doc["house"]["house_load_w"] = r.house_active_import_w;
    doc["house"]["pv_production_w"] = r.house_active_export_w;
  }
  doc["second"]["active_import_w"] = r.second_active_import_w;
  doc["second"]["active_export_w"] = r.second_active_export_w;
  doc["second"]["apparent_import_va"] = r.second_apparent_import_va;
  doc["second"]["apparent_export_va"] = r.second_apparent_export_va;
  doc["second"]["energy_day_import_wh"] = r.second_day_energy_import_wh;
  doc["second"]["energy_day_export_wh"] = r.second_day_energy_export_wh;
  doc["second"]["energy_total_import_wh"] = r.second_energy_import_wh;
  doc["second"]["energy_total_export_wh"] = r.second_energy_export_wh;
  JsonObject raw = doc["raw_meter"].to<JsonObject>();
  raw["voltage_house_v"] = r.house_voltage_v;
  raw["current_house_a"] = r.house_current_a;
  raw["pf_house"] = r.house_power_factor;
  raw["voltage_second_v"] = r.second_voltage_v;
  raw["current_second_a"] = r.second_current_a;
  raw["pf_second"] = r.second_power_factor;
  raw["freq_hz"] = r.mains_frequency_hz;
  raw["house_net_power_w"] = r.house_active_import_w - r.house_active_export_w;
  raw["second_net_power_w"] = r.second_active_import_w - r.second_active_export_w;
  if (balansun_cap_mqtt_linky_tariff() && LTARF.length() > 0) {
    doc["linky_tariff"] = LTARF;
  }
  JsonObject diag = doc["diagnostics"].to<JsonObject>();
  balansun_append_measurements_diagnostics(diag);
  balansun_temperature_set_primary_c_json(doc);
  balansun_temperature_append_telemetry_json(doc);
  doc["triac_sync_state"] =
      balansun_triac_sync_state_wire(balansun_triac_sync_state_from_zc(zc_sync_state, balansun_product_caps_compile_time()));
}
