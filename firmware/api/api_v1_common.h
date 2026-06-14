/*
 * api_v1_common.h — shared declarations for /api/v1 route modules.
 */
#ifndef API_V1_COMMON_H
#define API_V1_COMMON_H

#include <ArduinoJson.h>
#include "balansun_json.h"
#include <WebServer.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <Update.h>
#include <cctype>
#include <cstring>
#include <string>
#include "balansun_board.h"
#include "balansun_device_id.h"
#include "Actions.h"
#include "balansun_globals.h"
#include "api.h"
#include "api_util.h"
#include "balansun_api_ready.h"
#include "actions_api.h"
#include "actions_api_logic.h"
#include "balansun_pub.h"
#include "triac_api_shim.h"
#include "balansun_meter_json.h"
#include "balansun_meter_json_logic.h"
#include "balansun_source.h"
#include "balansun_mains_profile.h"
#include "balansun_install_countries.h"
#include "balansun_pwm.h"
#include "balansun_pwm_logic.h"
#include "balansun_status_led.h"
#include "fleet_bundle_logic.h"
#include "tempo_rte.h"
#include "balansun_forward.h"
#include "balansun_hw_profile.h"

extern WebServer server;
extern bool time_sync_valid;
extern int triacOverrideMaxTempC;
extern unsigned long ext_peer_ip;
extern unsigned int ext_peer_port;
extern unsigned long ext_peer_last_poll_ms;
extern bool ext_peer_last_poll_ok;
extern byte dhcpOn;
extern unsigned long IP_Fixe, Gateway, subnetMask, dns;
extern unsigned int mqtt_publish_period_sec;
extern unsigned long MQTTIP;
extern unsigned int MQTTPort;
extern unsigned int CalibU, CalibI;
extern float mains_frequency_hz;
extern int P_cent_EEPROM;
extern int RomUsedBytes;
extern float metering_task_ms_min, metering_task_ms_avg, metering_task_ms_max;
extern float previousLoopMin, previousLoopMoy, previousLoopMax;
#include "balansun_pwr_hist_limits.h"

extern int tabPwHouse_5mn[kBalansunPwrHist5mnSlots];
extern int tabPw_Triac_5mn[kBalansunPwrHist5mnSlots];
extern float tabTemperature_5mn[kBalansunPwrHist5mnSlots];
extern int IdxStockPW;
extern int tabPwHouse_2s[kBalansunPwrHist2sSlots];
extern int tabPw_Triac_2s[kBalansunPwrHist2sSlots];
extern int tabPvaHouse_2s[kBalansunPwrHist2sSlots];
extern int tabPva_Triac_2s[kBalansunPwrHist2sSlots];
extern float tabTemperature_2s[kBalansunPwrHist2sSlots];
extern int IdxStock2s;
extern int idxPromDuJour;
extern int NbActions;
extern PubSubClient clientMQTT;
extern unsigned long eeprom_layout_key;
extern bool meter_reading_valid;
extern float voltM[100];
extern float ampM[100];
extern float house_voltage_v;
extern float house_current_a;
extern float house_power_factor;
extern float second_voltage_v;
extern float second_current_a;
extern float second_power_factor;
extern float mains_frequency_hz;
extern double house_day_energy_import_wh;
extern double house_day_energy_export_wh;
extern double second_day_energy_import_wh;
extern double second_day_energy_export_wh;
extern int IdxDataRawLinky;
extern char DataRawLinky[10000];
extern bool tempoRteEnabled;
extern int enphase_house_active_w;
extern int enphase_production_w;
extern int shellyEmPollCounter;
extern float PwMQTT_last;
extern uint32_t JsyMk333SerialBaud;

extern int persistConfigToEeprom(void);
extern void balansun_init_action_gpios(void);
extern String eepromFormatYearlyEnergyHistory(void);
#include "balansun_reboot.h"
extern void eepromClearConsumptionHistory(void);

#ifndef kMaxRoutingActions
#define kMaxRoutingActions 20
#endif

static const size_t kPutBodyMax = 16384;
static const size_t kPatchBodyMax = 16384;
static const size_t kPmqttBindingsApplyCap = 8192;
/** JSON pool for GET /config and GET /system/backup with full pmqtt_bindings. */
static const size_t kConfigExportJsonCap = 49152;
static const int kHistDefaultMax = 200;
static const int kHistAbsMax = 600;
/** JSON cap for GET /api/v1/history/power (600 pts × 3 arrays needs ~32 KB pool on PSRAM). */
static const size_t kHistPowerJsonCap = 32768;
/** JSON cap for GET /api/v1/history/energy/daily (page + items). */
/** Max JSON pool for daily history page (proportional alloc uses less per request). */
static const size_t kHistEnergyDailyJsonCap = 16384;
static const size_t kHistEnergyDailyJsonPerRow = 320;
static const size_t kWifiBodyMax = 512;
static const size_t kTimeBodyMax = 512;
static const size_t kArduinoOtaBodyMax = 512;
static const size_t kHistoryDailyImportBodyMax = 32768;
static const int kHistoryDailyPageMax = 10;
static const int kEepromSize = 4090;
static const int kAdrParaActions = 1507;
static const size_t kHttpAuthBodyMax = 128;

#define API_AUTH_GUARD() \
  do {                 \
    if (!api_require_auth(server)) return; \
  } while (0)
#define API_AUTH_GUARD_R() \
  do {                     \
    if (!api_require_auth(server)) return true; \
  } while (0)

void api_append_config_object(JsonObject o);
bool config_apply_from_json(JsonObject root, bool fullPut, String &err);
void api_append_action_override(JsonObject o, int idx);
void api_append_measurements_object(JsonObject doc);
const char *override_state_name(byte state);
byte override_state_from_name(const char *state);

void handle_get_measurements();
void handle_get_telemetry_snapshot();
void handle_post_triac_override();
void handle_get_tariff_tempo();
void handle_get_system();
void handle_get_device();
void handle_get_state();
void handle_get_health();
void handle_get_system_audit();
void handle_post_health_self_test_run();
void handle_post_health_self_test_skip();
void handle_post_health_self_test_simulate();
void handle_post_health_hardware_recheck();
void handle_post_hardware_status_led_test();
void handle_get_sources();
void handle_get_gpio();
void handle_get_config();
void handle_put_config();
void handle_patch_config();
void handle_get_actions_live();
void handle_get_actions_schema();
void handle_get_actions_config();
void handle_put_actions_config();
void handle_patch_actions_config_batch();
void handle_get_action_override(int idx);
void handle_post_action_override(int idx);
void handle_clear_action_override(int idx);
void handle_get_history_power();
void handle_get_history_energy_daily();
void handle_post_history_energy_daily_import();
void handle_put_gpio();
void handle_get_pwm();
void handle_put_pwm();
void handle_get_fleet_export();
void handle_post_fleet_import();
void handle_put_fleet_trust_key();
void handle_post_reboot();
void handle_get_system_pins();
void handle_put_system_pins();
void handle_post_pins_reset();
void handle_get_wifi();
void handle_put_wifi();
void handle_get_wifi_scan();
void handle_post_factory_reset();
void handle_post_save_now();
void handle_get_eeprom();
void handle_get_time();
void handle_get_system_arduino_ota();
void handle_get_public();
void handle_put_system_http_auth();
void handle_get_system_backup();
void handle_put_system_backup();
bool Api_handle_backup_subresource();
void handle_post_auth_login();
void handle_post_auth_logout();
void handle_get_auth_tokens();
void handle_post_auth_tokens();
bool Api_handle_auth_tokens_subresource();
void handle_put_system_arduino_ota();
void handle_put_time();
void handle_post_firmware_ota_done();
void handle_firmware_ota_upload();
void handle_get_sources_diagnostics();
void handle_get_sources_brute_panel();
void handle_post_history_reset();
void handle_post_mqtt_discover();
void handle_post_mqtt_reconnect();
void handle_post_mqtt_publish_now();
void handle_post_mqtt_test();
void handle_post_pmqtt_preview();
void handle_post_sources_test_inject();
void handle_get_openapi();

#endif
