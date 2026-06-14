#pragma once

/*
 * balansun_string_limits.h — Max string lengths for fixed global buffers.
 * Derived from OpenAPI balansun-v1.yaml, EEPROM layout, and ESP WiFi limits.
 * Capacity includes trailing NUL (e.g. BALANSUN_WIFI_SSID_MAX = 33 → 32 chars + NUL).
 */

// WiFi / AP
constexpr unsigned BALANSUN_WIFI_SSID_MAX = 33;
constexpr unsigned BALANSUN_WIFI_PASSWORD_MAX = 65;

// Labels / probe names (OpenAPI)
constexpr unsigned BALANSUN_ROUTER_NAME_MAX = 64;
constexpr unsigned BALANSUN_PROBE_NAME_MAX = 64;
constexpr unsigned BALANSUN_TEMPERATURE_LABEL_MAX = 64;

// Source wire names (enum-like)
constexpr unsigned BALANSUN_SOURCE_NAME_MAX = 16;
constexpr unsigned BALANSUN_SOURCE_DATA_MAX = 16;

// Source Ext peer
constexpr unsigned BALANSUN_EXT_PEER_PATH_MAX = 49;  // API max 48 + NUL
constexpr unsigned BALANSUN_EXT_PEER_PROTOCOL_MAX = 16;
constexpr unsigned BALANSUN_EXT_PEER_POLL_ERR_MAX = 128;
constexpr unsigned BALANSUN_EXT_PEER_POLL_PREVIEW_MAX = 512;
constexpr unsigned BALANSUN_EXT_PEER_POLL_PROTOCOL_MAX = 16;

// MQTT HA discovery
constexpr unsigned BALANSUN_MQTT_FIELD_MAX = 64;
constexpr unsigned BALANSUN_MQTT_TOPIC_MAX = 128;
constexpr unsigned BALANSUN_MQTT_SCHEMA_MAX = 64;
constexpr unsigned BALANSUN_MQTT_BINDINGS_JSON_MAX = 4096;

// Secrets / auth
constexpr unsigned BALANSUN_HTTP_API_PASSWORD_MAX = 64;
constexpr unsigned BALANSUN_ARDUINO_OTA_PASSWORD_MAX = 64;
constexpr unsigned BALANSUN_FLEET_TRUST_KEY_MAX = 64;

// Enphase Envoy session
constexpr unsigned BALANSUN_ENPHASE_USER_MAX = 64;
constexpr unsigned BALANSUN_ENPHASE_PASSWORD_MAX = 64;
constexpr unsigned BALANSUN_ENPHASE_METER_CHANNEL_MAX = 32;
constexpr unsigned BALANSUN_ENPHASE_TOKEN_MAX = 256;
constexpr unsigned BALANSUN_ENPHASE_JSON_TOKEN_MAX = 256;
constexpr unsigned BALANSUN_ENPHASE_SESSION_ID_MAX = 128;

// Time / NTP
constexpr unsigned BALANSUN_TIME_TZ_MAX = 64;
constexpr unsigned BALANSUN_TIME_NTP_MAX = 64;
constexpr unsigned BALANSUN_SYNC_CLOCK_STR_MAX = 32;
constexpr unsigned BALANSUN_DATE_DDMMYYYY_MAX = 9;

// Linky / Tempo RTE
constexpr unsigned BALANSUN_LTARF_MAX = 16;
constexpr unsigned BALANSUN_STGE_TEMPO_MAX = 8;
constexpr unsigned BALANSUN_RTE_TEMPO_LABEL_MAX = 16;

// PWM mode label
constexpr unsigned BALANSUN_PWM_MODE_MAX = 16;

// Action wire format (Action class — wave 5; limits shared here)
constexpr unsigned BALANSUN_ACTION_TITLE_MAX = 64;
constexpr unsigned BALANSUN_ACTION_HOST_MAX = 64;
constexpr unsigned BALANSUN_ACTION_PATH_MAX = 128;

// Vendor brute-panel HTML (PSRAM-backed globals)
constexpr unsigned BALANSUN_RAW_DATA_PANEL_MAX = 8192;
