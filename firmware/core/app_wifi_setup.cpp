#include "app_wifi_setup.h"
#include "api_http_plain_body.h"
#include "balansun_http_service.h"
#include "captive_dns.h"
#include "balansun_forward.h"
#include "balansun_globals.h"
#include "balansun_reboot.h"

#include <balansun/platform/wifi_stack.h>

#include <ESPmDNS.h>
#include <WiFi.h>
#include <cstring>

#ifndef HOSTNAME
#define HOSTNAME "BALANSUN-"
#endif

void balansun_update_status_leds(void);

static void balansun_wifi_apply_hostname(void);

void balansun_wifi_prepare_hostname(void) {
  String host;
  balansun_platform_wifi_build_hostname(HOSTNAME, host);
  balansun_ap_ssid_storage.assign(host.c_str());
  ap_default_ssid = balansun_ap_ssid_storage.c_str();
  Serial.println(balansun_ap_ssid_storage.c_str());
}

bool balansun_wifi_load_sta_from_nvs(void) {
  String s;
  String p;
  if (!balansun_platform_wifi_nvs_load_sta(s, p)) return false;
  ssid.assign(s.c_str());
  password.assign(p.c_str());
  return true;
}

bool balansun_wifi_clear_sta_profile(void) { return balansun_platform_wifi_nvs_clear_sta(); }

bool balansun_wifi_save_sta_profile(void) {
  return balansun_platform_wifi_nvs_save_sta(String(ssid.c_str()), String(password.c_str()));
}

void balansun_wifi_prepare_stack(void) { balansun_platform_wifi_prepare_stack(); }

void balansun_wifi_connect_sta_or_ap(void) {
  Serial.println(String(F("SSID:")) + ssid.c_str());
  Serial.println(String(F("Pass:")) + password.c_str());
  if (ssid.length() == 0) {
    if (balansun_wifi_soft_ap_setup_active()) {
      Serial.println(F("Setup AP already active."));
      return;
    }
    Serial.println(F("No WiFi credentials - starting setup AP."));
    balansun_platform_wifi_start_soft_ap(ap_default_ssid, ap_default_psk, false);
    return;
  }
  Serial.println(F("Saved WiFi - starting setup AP, then joining station."));
  balansun_platform_wifi_start_soft_ap(ap_default_ssid, ap_default_psk, true);
  const unsigned long connect_start_ms = millis();
  constexpr unsigned long kStaConnectTimeoutMs = 15000UL;
  if (dhcpOn == 0) {
    byte arr[4];
    arr[0] = IP_Fixe & 0xFF;
    arr[1] = (IP_Fixe >> 8) & 0xFF;
    arr[2] = (IP_Fixe >> 16) & 0xFF;
    arr[3] = (IP_Fixe >> 24) & 0xFF;
    IPAddress local_IP(arr[3], arr[2], arr[1], arr[0]);
    arr[0] = Gateway & 0xFF;
    arr[1] = (Gateway >> 8) & 0xFF;
    arr[2] = (Gateway >> 16) & 0xFF;
    arr[3] = (Gateway >> 24) & 0xFF;
    IPAddress gateway(arr[3], arr[2], arr[1], arr[0]);
    arr[0] = subnetMask & 0xFF;
    arr[1] = (subnetMask >> 8) & 0xFF;
    arr[2] = (subnetMask >> 16) & 0xFF;
    arr[3] = (subnetMask >> 24) & 0xFF;
    IPAddress subnet(arr[3], arr[2], arr[1], arr[0]);
    arr[0] = dns & 0xFF;
    arr[1] = (dns >> 8) & 0xFF;
    arr[2] = (dns >> 16) & 0xFF;
    arr[3] = (dns >> 24) & 0xFF;
    IPAddress primaryDNS(arr[3], arr[2], arr[1], arr[0]);
    IPAddress secondaryDNS(8, 8, 4, 4);
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
      Serial.println("WIFI STA Failed to configure");
    }
  }
  Serial.println(String(F("Wifi Begin : ")) + ssid.c_str());
  WiFi.begin(ssid.c_str(), password.c_str());
  while (WiFi.status() != WL_CONNECTED && (millis() - connect_start_ms < kStaConnectTimeoutMs)) {
    balansun_reboot_poll();
    balansun_http_ensure_listening();
    for (int h = 0; h < 4; h++) {
      balansun_http_pump_server(server, 4);
      api_http_clear_plain_body();
      balansun_reboot_poll();
    }
    Serial.write('.');
    balansun_update_status_leds();
    Serial.print(WiFi.status());
    delay(300);
  }
  if (WiFi.status() == WL_CONNECTED) {
    balansun_captive_dns_stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    delay(50);
    balansun_wifi_apply_hostname();
    balansun_wifi_start_mdns();
    balansun_http_invalidate_binding();
    balansun_http_ensure_listening();
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println(String(F("Connected IP address: ")) + WiFi.localIP().toString() +
                   String(F(" or <a href='http://")) + balansun_ap_ssid_storage.c_str() + String(F(".local' >")) +
                   balansun_ap_ssid_storage.c_str() + String(F(".local</a>")));
  } else {
    Serial.println(F("Can not connect to WiFi station - setup AP remains active."));
    MDNS.end();
    balansun_http_ensure_listening();
    if (!balansun_captive_dns_active()) {
      balansun_captive_dns_start(WiFi.softAPIP());
    }
  }
}

bool balansun_wifi_soft_ap_setup_active(void) { return balansun_platform_wifi_soft_ap_setup_active(); }

void balansun_wifi_start_mdns(void) { balansun_platform_wifi_start_mdns(balansun_ap_ssid_storage.c_str()); }

static void balansun_wifi_apply_hostname(void) {
  balansun_platform_wifi_apply_hostname(balansun_ap_ssid_storage.c_str());
}
