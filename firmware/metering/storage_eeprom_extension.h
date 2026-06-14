#pragma once



#include "storage_eeprom_backend.h"

#include "storage_eeprom_layout.h"



#include <cstdint>

#include <functional>

#include <string>



/*

 * storage_eeprom_extension.h — Fields in the variable-length EEPROM extension tail.

 * Serialized by storage_eeprom_extension.cpp; loaded into balansun_globals via storage_eeprom.cpp.

 * REST: PUT/PATCH /api/v1/config (balansun_ext_*, pmqtt_*, pwm, fleet_trust_key, http_auth, …).

 */



/** Persisted extension settings (API field names in api_v1_routes config handlers). */

struct EepromExtensionFields {

  /** Pmqtt MQTT topic on discovery broker (MQTTIP). Max length enforced at write. */

  std::string pmqttTopic;

  /** Comma-separated JSON keys for Pw/Pva (default "Pw"). */

  std::string pmqttSchema = "Pw";

  /** JSON array string for per-metric Pmqtt bindings. */
  std::string pmqttBindingsJson = "[]";

  /** JSY-MK-333 UART baud on Serial2 (default 9600). */

  uint32_t jsyMk333SerialBaud = 9600;

  /** Source Ext HTTP port (default 80; 0 coerced to 80). */

  uint16_t ext_peer_port = 80;

  /** Source Ext snapshot path (default "/api/v1/measurements", max 48 chars). */

  std::string ext_peer_path = "/api/v1/measurements";

  /** Ext peer wire format: 1 = JSON (/api/v1/measurements). */

  uint8_t ext_peer_protocol_mode = 1;

  /** Dedicated PWM GPIO; -1 = disabled. See GUIDE B.3. */

  int8_t pwmGpio = -1;

  /** 0 off, 1 follow_triac, 2 independent. */

  uint8_t pwmMode = 0;

  /** PWM duty 0..100 (%). */

  uint8_t pwmDutyPercent = 0;

  bool pwmInverted = false;

  /** Fleet import HMAC key; empty rejects POST /api/v1/fleet/import. */

  std::string fleetTrustKey;

  /** ArduinoOTA password; empty = no OTA password. */

  std::string arduinoOtaPassword;

  /** HTTP API password; empty = open LAN. See /en/http-api-security/ */

  std::string httpApiPassword;

  /** Lab-only CORS on GET /api/v1 routes. */

  bool httpCorsEnabled = false;

  /** Fetch Tempo colors from RTE open data (non-Linky sources). */

  bool tempoRteEnabled = false;

  std::string tempoRteLtarfCache;

  std::string tempoRteStgeCache;

  std::string tempoRteJourCache;

  std::string tempoRteDemainCache;

  uint32_t tempoRteLastFetchEpoch = 0;

  /** Expert PID panel (0 = integral only). */
  uint8_t expertRegulationMode = 0;

  /** CACSI boost factor (1..99). */
  uint8_t regulationGain = 1;

  struct ActionRegCoeffs {
    uint8_t kp = 0;
    uint8_t ki = 4;
    uint8_t kd = 0;
    bool pid = false;
  };

  static constexpr int kRegCoeffsMax = 20;
  ActionRegCoeffs actionRegCoeffs[kRegCoeffsMax];

  bool regulationPersistPresent = false;

  /** HA site features (magic 0xE239). */
  bool haSitePersistPresent = false;
  bool vacationEnabled = false;
  uint32_t vacationEndEpoch = 0;
  uint16_t maxRoutedW = 0;
  bool mqttJsonCommands = false;
  bool triacOffWhenSourceStale = false;
  bool triacBackoffWhenHeaterIdle = false;
  uint32_t actionDailyCapWh[kRegCoeffsMax] = {};

  /** Serialized {"actions":[...]} blob (magic 0xE220). */
  std::string actionsJson;
  bool actionsJsonPresent = false;

  /** Legacy load profile block (magic 0xE211, tail before API tokens; read-only migration). */
  bool legacyLoadProfileBlockPresent = false;
  std::string loadProfilePersist = "water_heater_triac";

  /** Persisted API access tokens (magic 0xE20D, tail append). */
  struct ApiAccessTokenStored {
    uint8_t id = 0;
    std::string label;
    std::string token_hex;
  };
  static constexpr int kApiAccessTokenStoredMax = 4;
  ApiAccessTokenStored apiAccessTokens[kApiAccessTokenStoredMax];
  uint8_t apiAccessTokenCount = 0;

  /** Status LED settings (magic 0xE20E). */
  bool statusLedPersistPresent = false;
  uint8_t statusLedMode = 1;
  int8_t statusLedGpioActivity = 18;
  int8_t statusLedGpioRegulation = 19;
  int8_t statusLedRgbGpio = -1;
  bool statusLedActiveLow = false;
  uint8_t statusLedColorActivityR = 255;
  uint8_t statusLedColorActivityG = 180;
  uint8_t statusLedColorActivityB = 0;
  uint8_t statusLedColorRegulationR = 0;
  uint8_t statusLedColorRegulationG = 255;
  uint8_t statusLedColorRegulationB = 0;

  /** Advanced status LED overlay colors (magic 0xE20F). */
  bool statusLedAdvPersistPresent = false;

  /** DS18B20 bus GPIO and slots (magic 0xE210). */
  bool temperaturePersistPresent = false;
  int8_t tempGpio = -1;
  struct TemperatureSlotStored {
    bool enabled = false;
    std::string label;
    uint64_t address = 0;
  };
  static constexpr int kTemperatureSlotStoredMax = 2;
  TemperatureSlotStored temperatureSlots[kTemperatureSlotStoredMax];
  uint8_t statusLedColorRebootR = 255;
  uint8_t statusLedColorRebootG = 0;
  uint8_t statusLedColorRebootB = 0;
  uint8_t statusLedColorApR = 255;
  uint8_t statusLedColorApG = 105;
  uint8_t statusLedColorApB = 180;

};



using EepromMainsHook = std::function<int(int address)>;



/**

 * Read extension tail from @p address (after kEepromExtMagic).

 * Unknown magics are skipped (forward-compatible). @p read_mains may consume mains profile block.

 * @return next free address after last recognized block.

 */

int storage_eeprom_extension_read(int address, IEepromBackend &eeprom, EepromExtensionFields &fields,

                                  const EepromMainsHook &read_mains = nullptr);



/**

 * Write full extension chain (write order is the extension reference).

 * @p write_mains inserts mains profile between OTA and HTTP password blocks.

 */

int storage_eeprom_extension_write(int address, IEepromBackend &eeprom, const EepromExtensionFields &fields,

                                   const EepromMainsHook &write_mains = nullptr,

                                   bool *out_truncated = nullptr);

