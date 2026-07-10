#pragma once

#include <cstddef>
#include <cstdint>

/*
 * storage_eeprom_layout.h — Fixed EEPROM address map and extension-tail magic IDs.
 * Must stay in sync with storage_eeprom.cpp and storage_eeprom_extension.cpp.
 *
 * Fixed layout (4090 bytes emulated):
 *   [0 .. 1479]     annual fingerprint (kEepromNbJour × 4 bytes per day)
 *   [1480..1495]  day energy anchors (triac + house import/export at J0)
 *   [1496..1504]  currentDateStr string + lastStockConso marker
 *   [1507 .. ]    Balansun parameter stream: eeprom_layout_key, WiFi, MQTT, Source, actions, …
 *
 * Extension tail (variable, chained by uint16 magic after fixed parameter block):
 *   0xE200  kEepromExtMagic           — Pmqtt topic, schema, JsyMk333 serial baud
 *   0xE201  kEepromExtPortPathMagic   — Source Ext HTTP port + path
 *   0xE202  kEepromArduinoOtaPassMagic — ArduinoOTA password
 *   (hook)  read_mains / write_mains  — mains voltage profile
 *   0xE203  kEepromHttpApiPassMagic   — HTTP Basic API password
 *   0xE204  kEepromHttpCorsMagic      — lab CORS flag
 *   0xE205  kEepromPwmMagic           — dedicated PWM GPIO + mode
 *   0xE206  kEepromFleetTrustMagic    — fleet import HMAC key
 *   0xE207  kEepromTempoRteMagic      — RTE Tempo enable + cached calendar labels
 *   0xE208  kEepromRegulationV2Magic  — expert_regulation_mode, Kp/Ki/Kd/PID
 *   0xE209  kEepromHaSiteMagic        — vacation, caps, mqtt_json_commands
 *   0xE20A  kEepromDiagMagic          — self-test + triac calibration
 *   0xE220  kEepromActionsJsonMagic   — load_channels[] JSON (replaces previous binary action stream)
 *   0xE20C  kEepromPmqttBindingsMagic — Source Pmqtt per-metric bindings JSON
 *   0xE20D  kEepromHttpApiTokensMagic — persisted API access token secrets (tail append; was 0xE20B hashed)
 *   0xE20E  kEepromStatusLedMagic     — status LED mode, GPIO, RGB colors
 *   0xE20F  kEepromStatusLedAdvMagic  — reboot/AP overlay RGB colors
 *   0xE210  kEepromTemperatureMagic   — DS18B20 GPIO + two logical slots
 *   0xE211  kEepromWarmProfileMagic   — legacy load_profile tail (read-only migration; before tokens)
 *
 * See: /en/getting-started/ (EEPROM) and /en/developer/ § Release checklist (kEepromLayoutInit bump).
 */

constexpr int kEepromSize = 4090;
constexpr int kEepromNbJour = 370;
constexpr int kEepromAdrHistoAn = 0;
constexpr int kEepromAdrTriacImportJ0 = 1480;
constexpr int kEepromAdrETinjecte0 = 1484;
constexpr int kEepromAdrHouseImportJ0 = 1488;
constexpr int kEepromAdrEMinjecte0 = 1492;
constexpr int kEepromAdrcurrentDateStr = 1496;
constexpr int kEepromAdrLastStockConso = 1505;
constexpr int kEepromAdrParaActions = 1507;

constexpr uint16_t kEepromExtMagic = 0xE200u;
constexpr uint16_t kEepromExtPortPathMagic = 0xE201u;
constexpr uint16_t kEepromArduinoOtaPassMagic = 0xE202u;
constexpr uint16_t kEepromHttpApiPassMagic = 0xE203u;
constexpr uint16_t kEepromHttpCorsMagic = 0xE204u;
constexpr uint16_t kEepromPwmMagic = 0xE205u;
constexpr uint16_t kEepromFleetTrustMagic = 0xE206u;
constexpr uint16_t kEepromTempoRteMagic = 0xE207u;
constexpr uint16_t kEepromRegulationV2Magic = 0xE208u;
constexpr uint16_t kEepromHaSiteMagic = 0xE209u;
constexpr uint16_t kEepromDiagMagic = 0xE20Au;
constexpr uint16_t kEepromActionsJsonMagic = 0xE220u;
constexpr uint16_t kEepromPmqttBindingsMagic = 0xE20Cu;
constexpr uint16_t kEepromHttpApiTokensMagic = 0xE20Du;
constexpr uint16_t kEepromStatusLedMagic = 0xE20Eu;
constexpr uint16_t kEepromStatusLedAdvMagic = 0xE20Fu;
constexpr uint16_t kEepromTemperatureMagic = 0xE210u;
constexpr uint16_t kEepromWarmProfileMagic = 0xE211u;
constexpr uint16_t kEepromVictronGxMagic = 0xE212u;
constexpr unsigned kEepromLoadProfileMax = 32;
constexpr unsigned kEepromVictronPortalIdMax = 16;
constexpr unsigned kEepromTemperatureLabelMax = 64;
constexpr int kEepromApiAccessTokenSecretHexLen = 64;
constexpr int kEepromApiAccessTokenLabelMax = 24;
/** Max serialized actions JSON in extension tail (fits below kEepromSize with other blocks). */
constexpr int kEepromActionsJsonMax = 3072;

constexpr int kEepromTempoRteLabelMax = 31;

bool storage_eeprom_layout_validate();
uint32_t storage_eeprom_expected_cle_rom_init();
