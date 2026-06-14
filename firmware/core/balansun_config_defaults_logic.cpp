#include "balansun_config_defaults_logic.h"

#include "balansun_meter_pack.h"
#include "api_v1_common.h"
#include "balansun_forward.h"
#include "balansun_history_limits.h"
#include "balansun_hw_profile.h"
#include "balansun_mains_profile.h"
#include "balansun_regulation_modes.h"
#include "balansun_status_led_logic.h"
#include "balansun_diag.h"
#include "metering/pmqtt_bindings.h"

#include <cstring>

namespace {

constexpr const char *kFactorySource = BALANSUN_METER_PACK_FACTORY_SOURCE_WIRE;
constexpr const char *kFactoryMqttPrefix = "balansun";
constexpr const char *kFactoryRouterName = "Balansun";
constexpr const char *kFactoryProbeSecond = "Second channel";
constexpr const char *kFactoryProbeHouse = "House metering";
constexpr const char *kFactoryTempLabel = "temperature_c";
constexpr const char *kFactoryPmqttSchema = "Pw";
constexpr const char *kFactoryExtPath = "/api/v1/measurements";

bool ip_is_zero(unsigned long ip) { return ip == 0; }

bool status_led_block_is_factory() {
  if (statusLedMode != static_cast<uint8_t>(StatusLedMode::DualGpio)) return false;
  if (statusLedGpioActivity != 18 || statusLedGpioRegulation != 19) return false;
  if (statusLedRgbGpio != -1 || statusLedActiveLow) return false;
  if (statusLedColorActivity.r != 255 || statusLedColorActivity.g != 180 || statusLedColorActivity.b != 0) {
    return false;
  }
  if (statusLedColorRegulation.r != 0 || statusLedColorRegulation.g != 255 || statusLedColorRegulation.b != 0) {
    return false;
  }
  if (statusLedColorReboot.r != 255 || statusLedColorReboot.g != 0 || statusLedColorReboot.b != 0) return false;
  if (statusLedColorAp.r != 102 || statusLedColorAp.g != 0 || statusLedColorAp.b != 255) return false;
  return true;
}

bool triac_cal_is_factory() {
  if (g_triac_cal.enabled) return false;
  for (int i = 0; i < 3; i++) {
    if (g_triac_cal.points[i].duty_pct != 0 || g_triac_cal.points[i].measured_w != 0) return false;
  }
  return true;
}

bool action_daily_caps_factory() {
  for (int i = 0; i < NbActions; i++) {
    if (actionDailyCapWh[i] != 0) return false;
  }
  return true;
}

}  // namespace

bool balansun_backup_time_differs_from_factory() {
  if (TimeTz != "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00") return true;
  if (TimeNtp1 != "fr.pool.ntp.org") return true;
  if (TimeNtp2 != "time.nist.gov") return true;
  return false;
}

void balansun_backup_append_time(JsonObject timeObj, bool sparse) {
  if (sparse && !balansun_backup_time_differs_from_factory()) return;
  timeObj["tz"] = TimeTz;
  timeObj["ntp1"] = TimeNtp1;
  timeObj["ntp2"] = TimeNtp2;
}

bool balansun_config_global_key_is_factory_default(const char *key) {
  if (!key) return true;
  if (strcmp(key, "dhcp_on") == 0) return dhcpOn == 1;
  if (strcmp(key, "ip_fixed") == 0) return ip_is_zero(IP_Fixe);
  if (strcmp(key, "gateway") == 0) return ip_is_zero(Gateway);
  if (strcmp(key, "subnet_mask") == 0) return subnetMask == 4294967040UL;
  if (strcmp(key, "dns") == 0) return ip_is_zero(dns);
  if (strcmp(key, "source") == 0) return Source == kFactorySource;
  if (strcmp(key, "ext_peer_ip") == 0) return ip_is_zero(ext_peer_ip);
  if (strcmp(key, "ext_peer_port") == 0) return ext_peer_port == 80;
  if (strcmp(key, "ext_peer_path") == 0) return ext_peer_path == kFactoryExtPath;
  if (strcmp(key, "ext_protocol") == 0) return ext_peer_protocol == "json";
  if (strcmp(key, "enphase_user") == 0) return EnphaseUser.length() == 0;
  if (strcmp(key, "enphase_password") == 0) return EnphasePwd.length() == 0;
  if (strcmp(key, "meter_channel") == 0) return meter_channel.length() == 0;
  if (strcmp(key, "mqtt_repeat_sec") == 0) return mqtt_publish_period_sec == 0;
  if (strcmp(key, "mqtt_ip") == 0) return ip_is_zero(MQTTIP);
  if (strcmp(key, "mqtt_port") == 0) return MQTTPort == 1883;
  if (strcmp(key, "mqtt_user") == 0) return MQTTUser.length() == 0;
  if (strcmp(key, "mqtt_password") == 0) return MQTTPwd.length() == 0;
  if (strcmp(key, "mqtt_prefix") == 0) return MQTTPrefix == kFactoryMqttPrefix;
  if (strcmp(key, "mqtt_device_name") == 0) return MQTTdeviceName == kFactoryMqttPrefix;
  if (strcmp(key, "router_name") == 0) return routerName == kFactoryRouterName;
  if (strcmp(key, "probe_second_name") == 0) return probeSecondName == kFactoryProbeSecond;
  if (strcmp(key, "probe_house_name") == 0) return probeHouseName == kFactoryProbeHouse;
  if (strcmp(key, "temperature_label") == 0) return temperatureSensorName == kFactoryTempLabel;
  if (strcmp(key, "calib_u") == 0) return CalibU == 1000;
  if (strcmp(key, "calib_i") == 0) return CalibI == 1000;
  if (strcmp(key, "pmqtt_topic") == 0) return PmqttTopic.length() == 0;
  if (strcmp(key, "pmqtt_schema") == 0) return PmqttSchema == kFactoryPmqttSchema;
  if (strcmp(key, "jsy_mk333_serial_baud") == 0) return JsyMk333SerialBaud == 9600;
  if (strcmp(key, "install_country") == 0) {
    return strcmp(balansun_mains_install_country(), "FR") == 0;
  }
  if (strcmp(key, "install_country_variant") == 0) return balansun_mains_install_variant()[0] == '\0';
  if (strcmp(key, "mains_nominal_v") == 0) return balansun_mains_nominal_v() == 230;
  if (strcmp(key, "mains_frequency_mode") == 0) {
    return balansun_mains_frequency_mode() != MainsFrequencyMode::Manual;
  }
  if (strcmp(key, "mains_frequency_hz_manual") == 0) return balansun_mains_frequency_hz_manual() == 50;
  if (strcmp(key, "triac_override_max_temp_c") == 0) return triacOverrideMaxTempC == 0;
  if (strcmp(key, "http_cors_enabled") == 0) return !httpCorsEnabled;
  if (strcmp(key, "pwm_gpio") == 0) return pwmGpio == -1;
  if (strcmp(key, "pwm_mode") == 0) return pwmMode == "off" || pwmMode.length() == 0;
  if (strcmp(key, "pwm_duty_percent") == 0) return pwmDutyPercent == 0;
  if (strcmp(key, "pwm_inverted") == 0) return !pwmInverted;
  if (strcmp(key, "status_led_mode") == 0) return status_led_block_is_factory();
  if (strcmp(key, "status_led_gpio_activity") == 0) return status_led_block_is_factory();
  if (strcmp(key, "status_led_gpio_regulation") == 0) return status_led_block_is_factory();
  if (strcmp(key, "status_led_rgb_gpio") == 0) return status_led_block_is_factory();
  if (strcmp(key, "status_led_active_low") == 0) return status_led_block_is_factory();
  if (strcmp(key, "status_led_color_activity") == 0) return status_led_block_is_factory();
  if (strcmp(key, "status_led_color_regulation") == 0) return status_led_block_is_factory();
  if (strcmp(key, "status_led_color_reboot") == 0) return status_led_block_is_factory();
  if (strcmp(key, "status_led_color_ap") == 0) return status_led_block_is_factory();
  if (strcmp(key, "tempo_rte_enabled") == 0) return !tempoRteEnabled;
  if (strcmp(key, "expert_regulation_mode") == 0) return expert_regulation_mode == 0;
  if (strcmp(key, "regulation_gain") == 0) return regulation_gain == 1;
  if (strcmp(key, "triac_cal_enabled") == 0) return triac_cal_is_factory();
  if (strcmp(key, "triac_calibration") == 0) return triac_cal_is_factory();
  if (strcmp(key, "hunting_reversal_threshold") == 0) {
    return g_regulation_hunting_config.reversal_threshold == 3;
  }
  if (strcmp(key, "hunting_window_min") == 0) return g_regulation_hunting_config.window_min == 2;
  if (strcmp(key, "vacation_enabled") == 0) return !vacationEnabled;
  if (strcmp(key, "vacation_end_epoch") == 0) return vacationEndEpoch == 0;
  if (strcmp(key, "max_routed_w") == 0) return maxRoutedW == 0;
  if (strcmp(key, "mqtt_json_commands") == 0) return !mqttJsonCommands;
  if (strcmp(key, "triac_off_when_source_stale") == 0) return !triacOffWhenSourceStale;
  if (strcmp(key, "triac_backoff_when_heater_idle") == 0) return !triacBackoffWhenHeaterIdle;
  if (strcmp(key, "action_daily_cap_wh") == 0) return action_daily_caps_factory();
  return false;
}

void balansun_config_append_export(JsonObject o, bool sparse) {
  if (!sparse) {
    api_append_config_object(o);
    return;
  }

  const char *keys[] = {"dhcp_on",
                        "ip_fixed",
                        "gateway",
                        "subnet_mask",
                        "dns",
                        "source",
                        "ext_peer_ip",
                        "ext_peer_port",
                        "ext_peer_path",
                        "ext_protocol",
                        "enphase_user",
                        "enphase_password",
                        "meter_channel",
                        "mqtt_repeat_sec",
                        "mqtt_ip",
                        "mqtt_port",
                        "mqtt_user",
                        "mqtt_password",
                        "mqtt_prefix",
                        "mqtt_device_name",
                        "router_name",
                        "probe_second_name",
                        "probe_house_name",
                        "temperature_label",
                        "calib_u",
                        "calib_i",
                        "pmqtt_topic",
                        "pmqtt_schema",
                        "jsy_mk333_serial_baud",
                        "install_country",
                        "install_country_variant",
                        "mains_nominal_v",
                        "mains_frequency_mode",
                        "mains_frequency_hz_manual",
                        "triac_override_max_temp_c",
                        "http_cors_enabled",
                        "pwm_gpio",
                        "pwm_mode",
                        "pwm_duty_percent",
                        "pwm_inverted",
                        "tempo_rte_enabled",
                        "expert_regulation_mode",
                        "regulation_gain",
                        "triac_cal_enabled",
                        "hunting_reversal_threshold",
                        "hunting_window_min",
                        "vacation_enabled",
                        "vacation_end_epoch",
                        "max_routed_w",
                        "mqtt_json_commands",
                        "triac_off_when_source_stale",
                        "triac_backoff_when_heater_idle"};

  BalansunJsonDoc _balansunJsonPoolSparse = balansun_json_doc_alloc(8192);
  JsonDocument &fullDoc = _balansunJsonPoolSparse;
  JsonObject full = fullDoc.to<JsonObject>();
  api_append_config_object(full);

  for (const char *key : keys) {
    if (!balansun_config_global_key_is_factory_default(key) && !full[key].isNull()) {
      o[key] = full[key];
    }
  }

  if (!balansun_config_global_key_is_factory_default("status_led_mode")) {
    o["status_led_mode"] = full["status_led_mode"];
    o["status_led_gpio_activity"] = full["status_led_gpio_activity"];
    o["status_led_gpio_regulation"] = full["status_led_gpio_regulation"];
    o["status_led_rgb_gpio"] = full["status_led_rgb_gpio"];
    o["status_led_active_low"] = full["status_led_active_low"];
    o["status_led_color_activity"] = full["status_led_color_activity"];
    o["status_led_color_regulation"] = full["status_led_color_regulation"];
    o["status_led_color_reboot"] = full["status_led_color_reboot"];
    o["status_led_color_ap"] = full["status_led_color_ap"];
  }

  if (!triac_cal_is_factory()) {
    o["triac_cal_enabled"] = full["triac_cal_enabled"];
    o["triac_calibration"] = full["triac_calibration"];
  }

  if (!action_daily_caps_factory() && !full["action_daily_cap_wh"].isNull()) {
    o["action_daily_cap_wh"] = full["action_daily_cap_wh"];
  }

  if (meter_channel.length() > 0) {
    o["meter_channel"] = meter_channel;
  }
}

void balansun_config_append_export_metadata(JsonObject cfg, const BalansunHwCaps &caps) {
  const bool onDevice = PmqttBindingsJson.length() > 2;
  cfg["pmqtt_bindings_exported"] = caps.full_config_export && onDevice;
  cfg["pmqtt_bindings_on_device"] = onDevice;
}

bool config_merge_from_backup_json(JsonObject root, String &err) {
  return config_apply_from_json(root, false, err);
}

void balansun_backup_append_device_block(JsonObject device, const BalansunHwCaps &caps, bool sparse_export) {
  device["tier"] = balansun_hw_tier_name(balansun_hw_tier());
  device["full_config_export"] = caps.full_config_export;
  device["export_mode"] = sparse_export ? "sparse" : "full";
  device["defaults_profile"] = kBalansunConfigDefaultsProfile;
  device["config_export_max_bytes"] = caps.config_export_json_cap;
  device["put_body_max_bytes"] = caps.put_body_max;
  device["history_days_retained"] = kBalansunHistoryDaysRetained;
  device["firmware_version"] = Version;
}

bool balansun_backup_is_sparse_import(JsonObject root) {
  if (!root["device"].isNull()) {
    JsonObjectConst dev = root["device"];
    const char *mode = dev["export_mode"] | "";
    if (strcmp(mode, "sparse") == 0 || strcmp(mode, "sparse_parts") == 0) return true;
  }
  if (!root["config"].isNull()) {
    JsonObjectConst cfg = root["config"];
    if (!cfg["pmqtt_bindings_exported"].isNull() || !cfg["pmqtt_bindings_on_device"].isNull()) {
      return true;
    }
  }
  return false;
}
