#include "balansun_config_defaults_json.h"

#include "balansun_meter_pack.h"

#include <cstring>

namespace {

constexpr const char *kFactorySource = BALANSUN_METER_PACK_FACTORY_SOURCE_WIRE;
constexpr const char *kFactoryMqttPrefix = "balansun";
constexpr const char *kFactoryRouterName = "Balansun";
constexpr const char *kFactoryPmqttSchema = "Pw";

}  // namespace

bool balansun_config_value_is_factory_default(const char *key, JsonVariantConst value) {
  if (!key || value.isNull()) return true;
  if (strcmp(key, "dhcp_on") == 0) return value.as<bool>() == true;
  if (strcmp(key, "source") == 0) {
    const char *s = value.as<const char *>();
    return s && strcmp(s, kFactorySource) == 0;
  }
  if (strcmp(key, "calib_u") == 0 || strcmp(key, "calib_i") == 0) return value.as<int>() == 1000;
  if (strcmp(key, "mqtt_prefix") == 0 || strcmp(key, "mqtt_device_name") == 0) {
    const char *s = value.as<const char *>();
    return s && strcmp(s, kFactoryMqttPrefix) == 0;
  }
  if (strcmp(key, "router_name") == 0) {
    const char *s = value.as<const char *>();
    return s && strcmp(s, kFactoryRouterName) == 0;
  }
  if (strcmp(key, "pmqtt_schema") == 0) {
    const char *s = value.as<const char *>();
    return s && strcmp(s, kFactoryPmqttSchema) == 0;
  }
  if (strcmp(key, "install_country") == 0) {
    const char *s = value.as<const char *>();
    return s && strcmp(s, "FR") == 0;
  }
  if (strcmp(key, "mains_nominal_v") == 0) return value.as<int>() == 230;
  if (strcmp(key, "mains_frequency_mode") == 0) {
    const char *s = value.as<const char *>();
    return s && strcmp(s, "auto") == 0;
  }
  if (strcmp(key, "mains_frequency_hz_manual") == 0) return value.as<int>() == 50;
  if (strcmp(key, "ip_fixed") == 0 || strcmp(key, "gateway") == 0 || strcmp(key, "dns") == 0 ||
      strcmp(key, "mqtt_ip") == 0 || strcmp(key, "ext_peer_ip") == 0) {
    const char *s = value.as<const char *>();
    return s && strcmp(s, "0.0.0.0") == 0;
  }
  if (strcmp(key, "subnet_mask") == 0) {
    const char *s = value.as<const char *>();
    return s && strcmp(s, "255.255.255.0") == 0;
  }
  if (strcmp(key, "ext_peer_port") == 0 || strcmp(key, "mqtt_port") == 0) {
    return value.as<int>() == (strcmp(key, "mqtt_port") == 0 ? 1883 : 80);
  }
  if (strcmp(key, "jsy_mk333_serial_baud") == 0) return value.as<int>() == 9600;
  if (strcmp(key, "http_cors_enabled") == 0 || strcmp(key, "pwm_inverted") == 0) {
    return value.as<bool>() == false;
  }
  if (strcmp(key, "pwm_gpio") == 0) return value.as<int>() == -1;
  if (strcmp(key, "pwm_duty_percent") == 0 || strcmp(key, "triac_override_max_temp_c") == 0) {
    return value.as<int>() == 0;
  }
  if (strcmp(key, "pwm_mode") == 0) {
    const char *s = value.as<const char *>();
    return s && strcmp(s, "off") == 0;
  }
  return false;
}
