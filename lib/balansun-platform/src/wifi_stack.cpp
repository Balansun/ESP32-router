#include <balansun/platform/wifi_stack.h>

#include <balansun/platform/captive_dns.h>

#include <ESPmDNS.h>
#include <WiFi.h>
#include <cstring>
#include <esp_err.h>
#include <esp_wifi.h>
#include <nvs.h>

namespace {

constexpr int kSetupApChannel = 6;
constexpr const char kWifiNvsNamespace[] = "balansun_wifi";
constexpr const char kWifiNvsSsidKey[] = "ssid";
constexpr const char kWifiNvsPassKey[] = "pass";

IPAddress ip32_to_address(uint32_t ip) {
  return IPAddress((ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
}

void apply_country(void) {
  wifi_country_t country = {};
  strncpy(country.cc, "FR", sizeof(country.cc));
  country.schan = 1;
  country.nchan = 13;
  country.policy = WIFI_COUNTRY_POLICY_AUTO;
  const esp_err_t err = esp_wifi_set_country(&country);
  if (err != ESP_OK) {
    Serial.print(F("esp_wifi_set_country err="));
    Serial.println(static_cast<int>(err));
  }
}

}  // namespace

void balansun_platform_wifi_build_hostname(const char *prefix, String &hostname_out) {
  hostname_out = String(prefix);
  uint32_t chipId = 0;
  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  hostname_out += String(chipId);
}

void balansun_platform_wifi_prepare_stack(void) {
  WiFi.persistent(false);
  WiFi.setSleep(WIFI_PS_NONE);
  WiFi.useStaticBuffers(true);
  WiFi.mode(WIFI_OFF);
  delay(50);
  const bool modeOk = WiFi.mode(WIFI_STA);
  delay(100);
  WiFi.mode(WIFI_OFF);
  delay(50);
  Serial.print(F("WiFi stack ready="));
  Serial.println(modeOk ? F("yes") : F("no"));
}

bool balansun_platform_wifi_nvs_load_sta(String &ssid_out, String &password_out) {
  nvs_handle_t handle = 0;
  if (nvs_open(kWifiNvsNamespace, NVS_READONLY, &handle) != ESP_OK) {
    return false;
  }
  size_t ssidLen = 0;
  if (nvs_get_str(handle, kWifiNvsSsidKey, nullptr, &ssidLen) != ESP_OK || ssidLen <= 1) {
    nvs_close(handle);
    return false;
  }
  char ssidBuf[33] = {0};
  if (ssidLen > sizeof(ssidBuf) || nvs_get_str(handle, kWifiNvsSsidKey, ssidBuf, &ssidLen) != ESP_OK) {
    nvs_close(handle);
    return false;
  }
  size_t passLen = sizeof(char);
  char passBuf[65] = {0};
  if (nvs_get_str(handle, kWifiNvsPassKey, nullptr, &passLen) == ESP_OK && passLen <= sizeof(passBuf)) {
    nvs_get_str(handle, kWifiNvsPassKey, passBuf, &passLen);
  }
  nvs_close(handle);
  ssid_out = String(ssidBuf);
  password_out = String(passBuf);
  Serial.println(String(F("WiFi NVS profile: ")) + ssid_out.c_str());
  return true;
}

bool balansun_platform_wifi_nvs_save_sta(const String &ssid, const String &password) {
  const size_t ssidLen = ssid.length();
  const size_t passLen = password.length();
  if (ssidLen == 0 || ssidLen > 32 || passLen > 64) {
    return false;
  }
  nvs_handle_t handle = 0;
  if (nvs_open(kWifiNvsNamespace, NVS_READWRITE, &handle) != ESP_OK) {
    return false;
  }
  const bool ok = nvs_set_str(handle, kWifiNvsSsidKey, ssid.c_str()) == ESP_OK &&
                  nvs_set_str(handle, kWifiNvsPassKey, password.c_str()) == ESP_OK && nvs_commit(handle) == ESP_OK;
  nvs_close(handle);
  return ok;
}

bool balansun_platform_wifi_nvs_clear_sta(void) {
  nvs_handle_t handle = 0;
  if (nvs_open(kWifiNvsNamespace, NVS_READWRITE, &handle) != ESP_OK) {
    return false;
  }
  esp_err_t err = nvs_erase_all(handle);
  if (err == ESP_OK) err = nvs_commit(handle);
  nvs_close(handle);
  return err == ESP_OK;
}

bool balansun_platform_wifi_start_soft_ap(const char *ap_ssid, const char *ap_psk, bool ap_sta) {
  if (!ap_ssid || !ap_ssid[0]) return false;
  WiFi.persistent(false);
  WiFi.setSleep(WIFI_PS_NONE);
  WiFi.useStaticBuffers(true);
  const wifi_mode_t targetMode = ap_sta ? WIFI_MODE_APSTA : WIFI_MODE_AP;
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(targetMode);
  delay(200);
  const IPAddress apIp(192, 168, 4, 1);
  WiFi.softAPConfig(apIp, apIp, IPAddress(255, 255, 255, 0));
  const char *psk = (ap_psk && ap_psk[0]) ? ap_psk : nullptr;
  bool apOk = WiFi.softAP(ap_ssid, psk, kSetupApChannel, 0, 4);
  if (!apOk) {
    delay(300);
    WiFi.mode(targetMode);
    delay(200);
    WiFi.softAPConfig(apIp, apIp, IPAddress(255, 255, 255, 0));
    apOk = WiFi.softAP(ap_ssid, psk, kSetupApChannel, 0, 4);
  }
  if (apOk) apply_country();
  for (uint8_t i = 0; i < 25 && WiFi.softAPIP() == IPAddress(0, 0, 0, 0); ++i) {
    delay(50);
  }
  balansun_captive_dns_start(WiFi.softAPIP());
  Serial.print(F("Soft-AP "));
  Serial.print(ap_ssid);
  Serial.print(F(" @ "));
  Serial.println(WiFi.softAPIP().toString());
  return apOk;
}

bool balansun_platform_wifi_soft_ap_setup_active(void) {
  return WiFi.getMode() != WIFI_STA && WiFi.softAPIP() != IPAddress(0, 0, 0, 0);
}

void balansun_platform_wifi_apply_hostname(const char *hostname) {
  if (hostname && hostname[0]) {
    WiFi.setHostname(hostname);
  }
}

void balansun_platform_wifi_start_mdns(const char *hostname) {
  if (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0, 0, 0, 0)) return;
  if (!hostname || !hostname[0]) return;
  MDNS.end();
  if (!MDNS.begin(hostname)) {
    Serial.println(F("mDNS.begin failed"));
    return;
  }
  MDNS.addService("http", "tcp", 80);
  Serial.print(F("mDNS: http://"));
  Serial.print(hostname);
  Serial.println(F(".local/"));
}

bool balansun_platform_wifi_begin_sta(const String &ssid, const String &password, bool use_dhcp, uint32_t static_ip,
                                      uint32_t gateway, uint32_t subnet, uint32_t dns, unsigned long timeout_ms) {
  if (!use_dhcp) {
    const IPAddress local = ip32_to_address(static_ip);
    const IPAddress gw = ip32_to_address(gateway);
    const IPAddress mask = ip32_to_address(subnet);
    const IPAddress primary = ip32_to_address(dns);
    WiFi.config(local, gw, mask, primary, IPAddress(8, 8, 4, 4));
  }
  WiFi.begin(ssid.c_str(), password.c_str());
  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeout_ms) {
    delay(300);
  }
  return WiFi.status() == WL_CONNECTED;
}
