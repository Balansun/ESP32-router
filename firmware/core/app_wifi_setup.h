#pragma once
#include <Arduino.h>

/** Build `HOSTNAME` + efuse chip id; set `balansun_ap_ssid_storage` / `ap_default_ssid` (Wi-Fi hostname applied later). */
void balansun_wifi_prepare_hostname(void);

/** Init esp_wifi before large EEPROM heap alloc; does not start AP or join STA. */
void balansun_wifi_prepare_stack(void);

/** Load/save STA credentials via esp_wifi NVS profile (separate from EEPROM blob). */
bool balansun_wifi_load_sta_from_nvs(void);
bool balansun_wifi_save_sta_profile(void);
/** Remove persisted STA profile (balansun_wifi NVS namespace). */
bool balansun_wifi_clear_sta_profile(void);

/** After EEPROM load: connect STA (static IP if `dhcpOn==0`) or start soft AP. */
void balansun_wifi_connect_sta_or_ap(void);

/** True while the soft-AP is up for captive-portal / first-run Wi‑Fi configuration. */
bool balansun_wifi_soft_ap_setup_active(void);

/** Start mDNS responder (http://{hostname}.local) after STA has an IP. Safe to call repeatedly. */
void balansun_wifi_start_mdns(void);
