#pragma once

#include <Arduino.h>
#include <IPAddress.h>

/** Build `prefix` + efuse chip id (e.g. BALANSUN-12345678). */
void balansun_platform_wifi_build_hostname(const char *prefix, String &hostname_out);

/** Init esp_wifi before large heap alloc; does not join a network. */
void balansun_platform_wifi_prepare_stack(void);

bool balansun_platform_wifi_nvs_load_sta(String &ssid_out, String &password_out);
bool balansun_platform_wifi_nvs_save_sta(const String &ssid, const String &password);
bool balansun_platform_wifi_nvs_clear_sta(void);

/** Soft-AP at 192.168.4.1 + captive DNS. `ap_sta`: also enable STA radio (scan/join). */
bool balansun_platform_wifi_start_soft_ap(const char *ap_ssid, const char *ap_psk, bool ap_sta);

bool balansun_platform_wifi_soft_ap_setup_active(void);

void balansun_platform_wifi_apply_hostname(const char *hostname);
void balansun_platform_wifi_start_mdns(const char *hostname);

/** Join STA with optional static IP (pass 0.0.0.0 fields to use DHCP). */
bool balansun_platform_wifi_begin_sta(const String &ssid, const String &password, bool use_dhcp, uint32_t static_ip,
                                      uint32_t gateway, uint32_t subnet, uint32_t dns, unsigned long timeout_ms);
