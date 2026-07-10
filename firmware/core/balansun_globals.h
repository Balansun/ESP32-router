#pragma once

/*
 * balansun_globals.h — Process-wide mutable state (WiFi, metering, triac, MQTT, EEPROM mirrors).
 * Core 0 (balansun_metering_task) writes metering fields; core 1 reads via BalansunReadSnapshot() for HTTP/MQTT/triac.
 * Do not read house_active_import_w / history arrays from loop() without the snapshot API.
 * See: /en/project-overview/ § Global data; persisted fields map to storage_eeprom.cpp.
 */

#include "balansun_board.h"
#include "balansun_pin_map.h"
#include "balansun_pwr_hist_limits.h"
#include <Arduino.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "balansun_debug.h"
#include "Actions.h"
#include "balansun_fixed_str.h"
#include "balansun_status_led_logic.h"

// --- WiFi / AP setup ---
/** Holds AP SSID buffer while `ap_default_ssid` points at its `c_str()`. */
extern BalansunStr<BALANSUN_WIFI_SSID_MAX> balansun_ap_ssid_storage;
extern const char *ap_default_ssid;
extern const char *ap_default_psk;

/** EEPROM layout version key; must match kEepromLayoutInit after firmware upgrade. */
extern unsigned long eeprom_layout_key;
/** True when loadConfigFromEeprom() ran successfully this boot (params NVS is authoritative). */
extern bool balansun_eeprom_config_loaded;

extern BalansunStr<BALANSUN_WIFI_SSID_MAX> ssid;
extern BalansunStr<BALANSUN_WIFI_PASSWORD_MAX> password;
extern BalansunStr<BALANSUN_SOURCE_NAME_MAX> Source;
extern BalansunStr<BALANSUN_SOURCE_DATA_MAX> Source_data;
extern byte dhcpOn;
extern unsigned long IP_Fixe;
extern unsigned long Gateway;
extern unsigned long subnetMask;
extern unsigned long dns;

// --- Source Ext (Ext peer) — User: GUIDE A.6.1; See: /en/hardware-pinout/ source_ext ---
extern unsigned long ext_peer_ip;
/** TCP port for Source Ext remote HTTP (default 80). REST: ext_peer_port */
extern unsigned int ext_peer_port;
/** HTTP path for Source Ext snapshot, e.g. /api/v1/measurements (max 48 chars). */
extern BalansunStr<BALANSUN_EXT_PEER_PATH_MAX> ext_peer_path;
/** Ext peer protocol (always JSON snapshot). */
extern BalansunStr<BALANSUN_EXT_PEER_PROTOCOL_MAX> ext_peer_protocol;
extern unsigned long ext_peer_last_poll_ms;
extern bool ext_peer_last_poll_ok;
extern BalansunStr<BALANSUN_EXT_PEER_POLL_ERR_MAX> ext_peer_last_poll_err;
extern BalansunStr<BALANSUN_EXT_PEER_POLL_PREVIEW_MAX> ext_peer_last_poll_preview;
extern BalansunStr<BALANSUN_EXT_PEER_POLL_PROTOCOL_MAX> ext_peer_last_poll_protocol;
/** Count of Ajax fallbacks in auto mode (for deprecation telemetry). */
extern bool LinkyEaitFromTic;
extern bool LinkySinstiSeen;
/** Lab CORS on GET /api/v1/*; See: /en/http-api-security/ */
extern bool httpCorsEnabled;

// --- Dedicated PWM (optional hardware channel) — User: GUIDE B.3 ---
/** Dedicated PWM output (-1 = disabled). */
extern int pwmGpio;
extern BalansunStr<BALANSUN_PWM_MODE_MAX> pwmMode;
extern int pwmDutyPercent;
extern bool pwmInverted;
/** Status LED configuration — REST: status_led_* */
extern uint8_t statusLedMode;
extern bool statusLedPersistPresent;
extern int8_t statusLedGpioActivity;
extern int8_t statusLedGpioRegulation;
extern int8_t statusLedRgbGpio;
extern bool statusLedActiveLow;
extern StatusLedRgb statusLedColorActivity;
extern StatusLedRgb statusLedColorRegulation;
extern StatusLedRgb statusLedColorReboot;
extern StatusLedRgb statusLedColorAp;
extern bool statusLedAdvPersistPresent;
/** HMAC key for fleet import (empty = import rejected). */
extern BalansunStr<BALANSUN_FLEET_TRUST_KEY_MAX> fleetTrustKey;

/** HTTP path for Ext main snapshot (default /api/v1/measurements). */
String ext_peer_main_data_path();
/** Path for raw meter / brute panel proxy. */
String ext_peer_raw_data_path(int lastIdx);

// --- MQTT Home Assistant discovery + telemetry ---
extern unsigned int mqtt_publish_period_sec;
extern unsigned long MQTTIP;
extern unsigned int MQTTPort;
extern BalansunStr<BALANSUN_MQTT_FIELD_MAX> MQTTUser;
extern BalansunStr<BALANSUN_MQTT_FIELD_MAX> MQTTPwd;
extern BalansunStr<BALANSUN_MQTT_FIELD_MAX> MQTTPrefix;
extern BalansunStr<BALANSUN_MQTT_FIELD_MAX> MQTTdeviceName;
/** ArduinoOTA (IDE / PlatformIO) network password; empty = no password. */
extern BalansunStr<BALANSUN_ARDUINO_OTA_PASSWORD_MAX> arduinoOtaPassword;
/** HTTP API Basic Auth password; empty = open LAN (no auth). */
extern BalansunStr<BALANSUN_HTTP_API_PASSWORD_MAX> httpApiPassword;

// --- Labels (MQTT HA discovery, UI) ---
extern BalansunStr<BALANSUN_ROUTER_NAME_MAX> routerName;
extern BalansunStr<BALANSUN_PROBE_NAME_MAX> probeSecondName;
extern BalansunStr<BALANSUN_PROBE_NAME_MAX> probeHouseName;
extern BalansunStr<BALANSUN_TEMPERATURE_LABEL_MAX> temperatureSensorName;
extern int P_cent_EEPROM;
extern int RomUsedBytes;
extern int cptLEDyellow;
extern int cptLEDgreen;

// --- Analog calibration (ADC) — See: /en/build/pinout/sources/analog source_analog ---
extern unsigned int CalibU;
extern unsigned int CalibI;
extern int value0;
extern int volt[100];
extern int amp[100];
extern float KV;
extern float KI;
extern float kV;
extern float kI;
extern float voltM[100];
extern float ampM[100];

extern bool meter_reading_valid;
extern long EAS_T_J0;
extern long EAI_T_J0;
extern long EAS_M_J0;
extern long EAI_M_J0;
/** Same-calendar-day floors — prevent day counters stepping backward after OTA. */
extern long g_day_floor_house_import_wh;
extern long g_day_floor_house_export_wh;
extern long g_day_floor_second_import_wh;
extern long g_day_floor_second_export_wh;
/** VictronGx: true after first successful JSY CH2 poll this boot. */
extern bool g_second_channel_meter_valid_this_boot;

extern int adr_debut_para;

// --- Live metering (house _M / triac _T) — sign: import positive per GUIDE A.2 ---
extern float second_voltage_v, second_current_a, second_power_factor, mains_frequency_hz;
extern float house_voltage_v, house_current_a, house_power_factor;
/* Energy counters are double: float (24-bit mantissa) loses 1 Wh resolution above ~16.7 MWh,
 * which a house incomer reaches in well under a year. double is exact to ~9e15 Wh. */
extern double second_energy_import_wh;
extern double second_energy_export_wh;
extern double house_energy_import_wh;
extern double house_energy_export_wh;
extern double second_day_energy_export_wh;
extern double house_day_energy_export_wh;
extern double second_day_energy_import_wh;
extern double house_day_energy_import_wh;
extern int second_active_import_w, house_active_import_w, second_active_export_w, house_active_export_w;
extern int second_apparent_import_va, house_apparent_import_va, second_apparent_export_va, house_apparent_export_va;
extern int enphase_house_active_w, enphase_production_w;
extern int tabPwHouse_5mn[kBalansunPwrHist5mnSlots];
extern int tabPw_Triac_5mn[kBalansunPwrHist5mnSlots];
extern float tabTemperature_5mn[kBalansunPwrHist5mnSlots];
extern int tabPwHouse_2s[kBalansunPwrHist2sSlots];
extern int tabPw_Triac_2s[kBalansunPwrHist2sSlots];
extern int tabPvaHouse_2s[kBalansunPwrHist2sSlots];
extern int tabPva_Triac_2s[kBalansunPwrHist2sSlots];
extern float tabTemperature_2s[kBalansunPwrHist2sSlots];
extern int IdxStock2s;
extern int IdxStockPW;

// --- Linky TIC decode buffers ---
extern byte ByteArray[130];
extern long LesDatas[14];
extern int Sens_1, Sens_2;

extern bool LFon;
extern bool EASTvalid;
extern bool EAITvalid;
extern int IdxDataRawLinky;
extern int IdxBufDecodLinky;
extern char DataRawLinky[10000];
extern float moyPWS;
extern float moyPWI;
extern float moyPVAS;
extern float moyPVAI;
extern float COSphiS;
extern float COSphiI;
extern long TlastEASTvalide;
extern long TlastEAITvalide;
extern BalansunStr<BALANSUN_LTARF_MAX> LTARF;
/** Tomorrow Tempo nibble from Linky STGE or RTE (hex digit string: 4/8/C). */
extern BalansunStr<BALANSUN_STGE_TEMPO_MAX> STGEt;
extern BalansunStr<BALANSUN_RTE_TEMPO_LABEL_MAX> rte_today;
extern BalansunStr<BALANSUN_RTE_TEMPO_LABEL_MAX> rte_tomorrow;
extern bool tempoRteEnabled;
extern int tempoRteLastPollDecihours;
extern int LTARFbin;
extern uint32_t tempoRteLastFetchEpoch;

// --- Enphase Envoy API session ---
extern BalansunStr<BALANSUN_ENPHASE_TOKEN_MAX> TokenEnphase;
extern BalansunStr<BALANSUN_ENPHASE_USER_MAX> EnphaseUser;
extern BalansunStr<BALANSUN_ENPHASE_PASSWORD_MAX> EnphasePwd;
extern BalansunStr<BALANSUN_ENPHASE_METER_CHANNEL_MAX> meter_channel;
extern BalansunStr<BALANSUN_ENPHASE_JSON_TOKEN_MAX> JsonToken;
extern BalansunStr<BALANSUN_ENPHASE_SESSION_ID_MAX> Session_id;
extern long LastwhDlvdCum;
extern float EMI_Wh;
extern float EMS_Wh;

extern BalansunPsramStr<BALANSUN_RAW_DATA_PANEL_MAX> SG_rawData;
extern BalansunPsramStr<BALANSUN_RAW_DATA_PANEL_MAX> ShEm_rawData;
extern int shellyEmPollCounter;
/** Vendor raw panel HTML fragments for brute-data page (HomeWizard, Shelly Pro EM, JSY-MK-333). */
extern BalansunPsramStr<BALANSUN_RAW_DATA_PANEL_MAX> HW_rawData;
extern BalansunPsramStr<BALANSUN_RAW_DATA_PANEL_MAX> ShPro_rawData;
extern BalansunPsramStr<BALANSUN_RAW_DATA_PANEL_MAX> MK333_rawData;

// --- Source Pmqtt — User: GUIDE A.7; subscribes on MQTTIP broker ---
/** Source Pmqtt: subscribe to this topic on the same broker as HA discovery (MQTTIP). */
extern BalansunStr<BALANSUN_MQTT_TOPIC_MAX> PmqttTopic;
/** Comma-separated keys to parse from JSON payload (e.g. "Pw" or "Pw,Pva"). */
extern BalansunStr<BALANSUN_MQTT_SCHEMA_MAX> PmqttSchema;
/** JSON array persisted in config (`pmqtt_bindings`). */
extern BalansunPsramStr<BALANSUN_MQTT_BINDINGS_JSON_MAX> PmqttBindingsJson;
extern float PwMQTT_last;
extern unsigned long LastPwMQTTMillis;
extern uint32_t JsyMk333SerialBaud;

// --- Source Victron Cerbo GX (2nd MQTT client to Cerbo broker) ---
extern unsigned long victron_broker_ip;
extern BalansunStr<BALANSUN_VICTRON_PORTAL_ID_MAX> VictronPortalId;
extern uint16_t victron_battery_device_id;
extern BalansunStr<BALANSUN_VICTRON_SURPLUS_MODE_MAX> VictronSurplusModeWire;
extern uint8_t victron_grid_phases;
extern unsigned long LastVictronMqttMillis;

// --- Routing actions (index 0 = triac channel in regulation) ---
extern Action load_channels[kMaxRoutingActions];
extern int NbActions;

/** 0 = standard (integral), 1 = expert PID UI. */
extern uint8_t expert_regulation_mode;
/** CACSI reactivity boost 1..99. */
extern uint8_t regulation_gain;

/** Vacation mode (IDEA-U3): suspend regulation when active. */
extern bool vacationEnabled;
extern uint32_t vacationEndEpoch;
/** Site-level max routed power (W); 0 = disabled (IDEA-S4). */
extern uint16_t maxRoutedW;
extern bool siteCapActive;
/** Per-action daily routed energy cap (Wh); 0 = disabled (IDEA-S3). */
extern uint32_t actionDailyCapWh[kMaxRoutingActions];
extern bool actionCapHit[kMaxRoutingActions];
/** Allow MQTT JSON config topics (IDEA-H7 / H2-W2). */
extern bool mqttJsonCommands;
/** When true, force triac off while source health is stale (split Ext installs). */
extern bool triacOffWhenSourceStale;
/** When true, self-test blocks triac/regulation outputs (commissioning). */
extern bool commissioningBlocksOutputs;
extern bool triacBackoffWhenHeaterIdle;
/** True while CH2 load feedback forces triac off (tank thermostat likely open). */
extern bool heaterLoadBackoffActive;
/** Load profile wire (see OpenAPI LoadProfile). */
extern BalansunStr<32> loadProfileWire;

// --- Timing / diagnostics (loop and metering task profiling) ---
extern unsigned long startMillis;
extern unsigned long previousWifiMillis;
extern unsigned long previousHistoryMillis;
extern unsigned long previousWsMillis;
extern unsigned long previousWiMillis;
extern unsigned long last_metering_task_ms;
extern unsigned long previousTimer2sMillis;
extern unsigned long previousOverProdMillis;
extern unsigned long previousLEDsMillis;
extern unsigned long previousLoop;
extern unsigned long previousETX;
extern unsigned long poll_period_ms;
extern float previousLoopMin;
extern float previousLoopMax;
extern float previousLoopMoy;
extern unsigned long last_metering_task_at_ms;
extern float metering_task_ms_min;
extern float metering_task_ms_max;
extern float metering_task_ms_avg;
extern unsigned long previousMqttMillis;

/** Triac phase delay (control path; ISRs use triac_delay_percent/triac_delay_percent_f in balansun_triac_isr). */
extern float triac_delay_percent_f;

// --- HTTP server, time, temperature ---
extern WebServer server;

extern const char *ntpServer1;
extern const char *ntpServer2;
extern BalansunStr<BALANSUN_TIME_TZ_MAX> TimeTz;
extern BalansunStr<BALANSUN_TIME_NTP_MAX> TimeNtp1;
extern BalansunStr<BALANSUN_TIME_NTP_MAX> TimeNtp2;
extern BalansunStr<BALANSUN_SYNC_CLOCK_STR_MAX> sync_clock_str;
extern BalansunStr<BALANSUN_DATE_DDMMYYYY_MAX> currentDateStr;
extern bool time_sync_valid;
extern int wall_clock_decihours;
extern int idxPromDuJour;

extern float temperature;
/** 0 = disable; default 70 — blocks triac_fixed 100% override when probe reads hotter. */
extern int triacOverrideMaxTempC;

#if BALANSUN_REMOTE_DEBUG
extern RemoteDebug Debug;
#else
extern RmsDebugStub Debug;
#endif
extern WiFiClient MqttClient;
extern PubSubClient clientMQTT;
/** STA disconnect streak (main loop); triggers reconnect / reboot — not meter HTTP failures. */
extern int WIFIbug;
/** Failed HTTP to external meter peers (Shelly, Ext, …); logged only — does not affect Wi‑Fi. */
extern int meterPeerFailures;

extern TaskHandle_t Task1;
