/*
 * balansun_globals.cpp — Default values for globals declared in balansun_globals.h.
 * Lab-only compile-time WiFi/MQTT defaults via BALANSUN_DEFAULT_* build_flags (see FIRMWARE_BUILD.md).
 */
#include "balansun_globals.h"
#include "balansun_board.h"
#include "balansun_meter_pack.h"

#ifndef BALANSUN_DEFAULT_WIFI_SSID
#define BALANSUN_DEFAULT_WIFI_SSID ""
#endif
#ifndef BALANSUN_DEFAULT_WIFI_PASSWORD
#define BALANSUN_DEFAULT_WIFI_PASSWORD ""
#endif
#ifndef BALANSUN_DEFAULT_MQTT_USER
#define BALANSUN_DEFAULT_MQTT_USER ""
#endif
#ifndef BALANSUN_DEFAULT_MQTT_PASSWORD
#define BALANSUN_DEFAULT_MQTT_PASSWORD ""
#endif

const char *ap_default_ssid = nullptr;
const char *ap_default_psk = nullptr;

unsigned long eeprom_layout_key = kEepromLayoutInit;
bool balansun_eeprom_config_loaded = false;

BalansunStr<BALANSUN_WIFI_SSID_MAX> balansun_ap_ssid_storage;

BalansunStr<BALANSUN_WIFI_SSID_MAX> ssid = BALANSUN_DEFAULT_WIFI_SSID;
BalansunStr<BALANSUN_WIFI_PASSWORD_MAX> password = BALANSUN_DEFAULT_WIFI_PASSWORD;
BalansunStr<BALANSUN_SOURCE_NAME_MAX> Source = BALANSUN_METER_PACK_FACTORY_SOURCE_WIRE;
BalansunStr<BALANSUN_SOURCE_DATA_MAX> Source_data = BALANSUN_METER_PACK_FACTORY_SOURCE_WIRE;
byte dhcpOn = 1;
unsigned long IP_Fixe = 0;
unsigned long Gateway = 0;
unsigned long subnetMask = 4294967040;
unsigned long dns = 0;
unsigned long ext_peer_ip = 0;
unsigned int ext_peer_port = 80;
BalansunStr<BALANSUN_EXT_PEER_PATH_MAX> ext_peer_path = "/api/v1/measurements";
BalansunStr<BALANSUN_EXT_PEER_PROTOCOL_MAX> ext_peer_protocol = "json";
unsigned long ext_peer_last_poll_ms = 0;
bool ext_peer_last_poll_ok = false;
BalansunStr<BALANSUN_EXT_PEER_POLL_ERR_MAX> ext_peer_last_poll_err;
BalansunStr<BALANSUN_EXT_PEER_POLL_PREVIEW_MAX> ext_peer_last_poll_preview;
BalansunStr<BALANSUN_EXT_PEER_POLL_PROTOCOL_MAX> ext_peer_last_poll_protocol;
bool LinkyEaitFromTic = false;
bool LinkySinstiSeen = false;
bool httpCorsEnabled = false;
int pwmGpio = -1;
BalansunStr<BALANSUN_PWM_MODE_MAX> pwmMode = "off";
int pwmDutyPercent = 0;
bool pwmInverted = false;
uint8_t statusLedMode = static_cast<uint8_t>(StatusLedMode::DualGpio);
bool statusLedPersistPresent = false;
int8_t statusLedGpioActivity = 18;
int8_t statusLedGpioRegulation = 19;
int8_t statusLedRgbGpio = -1;
bool statusLedActiveLow = false;
StatusLedRgb statusLedColorActivity(255, 180, 0);
StatusLedRgb statusLedColorRegulation(0, 255, 0);
StatusLedRgb statusLedColorReboot(255, 0, 0);
StatusLedRgb statusLedColorAp(102, 0, 255);
bool statusLedAdvPersistPresent = false;
BalansunStr<BALANSUN_FLEET_TRUST_KEY_MAX> fleetTrustKey;
unsigned int mqtt_publish_period_sec = 0;
unsigned long MQTTIP = 0;
unsigned int MQTTPort = 1883;
BalansunStr<BALANSUN_MQTT_FIELD_MAX> MQTTUser = BALANSUN_DEFAULT_MQTT_USER;
BalansunStr<BALANSUN_MQTT_FIELD_MAX> MQTTPwd = BALANSUN_DEFAULT_MQTT_PASSWORD;
BalansunStr<BALANSUN_MQTT_FIELD_MAX> MQTTPrefix = "balansun";
BalansunStr<BALANSUN_MQTT_FIELD_MAX> MQTTdeviceName = "balansun";
BalansunStr<BALANSUN_ARDUINO_OTA_PASSWORD_MAX> arduinoOtaPassword;
BalansunStr<BALANSUN_HTTP_API_PASSWORD_MAX> httpApiPassword;
int RomUsedBytes = 0;
BalansunStr<BALANSUN_ROUTER_NAME_MAX> routerName = "Balansun";
BalansunStr<BALANSUN_PROBE_NAME_MAX> probeSecondName = "Second channel";
BalansunStr<BALANSUN_PROBE_NAME_MAX> probeHouseName = "House metering";
BalansunStr<BALANSUN_TEMPERATURE_LABEL_MAX> temperatureSensorName = "temperature_c";
int P_cent_EEPROM;
int cptLEDyellow = 0;
int cptLEDgreen = 0;

unsigned int CalibU = 1000;
unsigned int CalibI = 1000;
int value0;
int volt[100];
int amp[100];
float KV = 0.2083f;
float KI = 0.0642f;
float kV = 0.2083f;
float kI = 0.0642f;
float voltM[100];
float ampM[100];

bool meter_reading_valid = false;
long EAS_T_J0 = 0;
long EAI_T_J0 = 0;
long EAS_M_J0 = 0;
long EAI_M_J0 = 0;
long g_day_floor_house_import_wh = 0;
long g_day_floor_house_export_wh = 0;
long g_day_floor_second_import_wh = 0;
long g_day_floor_second_export_wh = 0;
bool g_second_channel_meter_valid_this_boot = false;

int adr_debut_para = 0;

float second_voltage_v, second_current_a, second_power_factor, mains_frequency_hz;
float house_voltage_v, house_current_a, house_power_factor;
double second_energy_import_wh = 0;
double second_energy_export_wh = 0;
double house_energy_import_wh = 0;
double house_energy_export_wh = 0;
double second_day_energy_export_wh = 0;
double house_day_energy_export_wh = 0;
double second_day_energy_import_wh = 0;
double house_day_energy_import_wh = 0;
int second_active_import_w, house_active_import_w, second_active_export_w, house_active_export_w;
int second_apparent_import_va, house_apparent_import_va, second_apparent_export_va, house_apparent_export_va;
int enphase_house_active_w, enphase_production_w;
int tabPwHouse_5mn[kBalansunPwrHist5mnSlots];
int tabPw_Triac_5mn[kBalansunPwrHist5mnSlots];
float tabTemperature_5mn[kBalansunPwrHist5mnSlots];
int tabPwHouse_2s[kBalansunPwrHist2sSlots];
int tabPw_Triac_2s[kBalansunPwrHist2sSlots];
int tabPvaHouse_2s[kBalansunPwrHist2sSlots];
int tabPva_Triac_2s[kBalansunPwrHist2sSlots];
float tabTemperature_2s[kBalansunPwrHist2sSlots];
int IdxStock2s = 0;
int IdxStockPW = 0;

byte ByteArray[130];
long LesDatas[14];
int Sens_1, Sens_2;

bool LFon = false;
bool EASTvalid = false;
bool EAITvalid = false;
int IdxDataRawLinky = 0;
int IdxBufDecodLinky = 0;
char DataRawLinky[10000];
float moyPWS = 0;
float moyPWI = 0;
float moyPVAS = 0;
float moyPVAI = 0;
float COSphiS = 1;
float COSphiI = 1;
long TlastEASTvalide = 0;
long TlastEAITvalide = 0;
BalansunStr<BALANSUN_LTARF_MAX> LTARF;
BalansunStr<BALANSUN_STGE_TEMPO_MAX> STGEt;
BalansunStr<BALANSUN_RTE_TEMPO_LABEL_MAX> rte_today = "UNDEFINED";
BalansunStr<BALANSUN_RTE_TEMPO_LABEL_MAX> rte_tomorrow = "UNDEFINED";
bool tempoRteEnabled = false;
int tempoRteLastPollDecihours = -1;
int LTARFbin = 0;
uint32_t tempoRteLastFetchEpoch = 0;

BalansunStr<BALANSUN_ENPHASE_TOKEN_MAX> TokenEnphase;
BalansunStr<BALANSUN_ENPHASE_USER_MAX> EnphaseUser;
BalansunStr<BALANSUN_ENPHASE_PASSWORD_MAX> EnphasePwd;
BalansunStr<BALANSUN_ENPHASE_METER_CHANNEL_MAX> meter_channel = "0";
BalansunStr<BALANSUN_ENPHASE_JSON_TOKEN_MAX> JsonToken;
BalansunStr<BALANSUN_ENPHASE_SESSION_ID_MAX> Session_id;
long LastwhDlvdCum = 0;
float EMI_Wh = 0;
float EMS_Wh = 0;

BalansunPsramStr<BALANSUN_RAW_DATA_PANEL_MAX> SG_rawData;
BalansunPsramStr<BALANSUN_RAW_DATA_PANEL_MAX> ShEm_rawData;
int shellyEmPollCounter = 0;
BalansunPsramStr<BALANSUN_RAW_DATA_PANEL_MAX> HW_rawData;
BalansunPsramStr<BALANSUN_RAW_DATA_PANEL_MAX> ShPro_rawData;
BalansunPsramStr<BALANSUN_RAW_DATA_PANEL_MAX> MK333_rawData;
BalansunStr<BALANSUN_MQTT_TOPIC_MAX> PmqttTopic;
BalansunStr<BALANSUN_MQTT_SCHEMA_MAX> PmqttSchema = "Pw";
BalansunPsramStr<BALANSUN_MQTT_BINDINGS_JSON_MAX> PmqttBindingsJson;
float PwMQTT_last = 0;
unsigned long LastPwMQTTMillis = 0;
uint32_t JsyMk333SerialBaud = 9600;

unsigned long victron_broker_ip = 0;
BalansunStr<BALANSUN_VICTRON_PORTAL_ID_MAX> VictronPortalId;
uint16_t victron_battery_device_id = 0;
BalansunStr<BALANSUN_VICTRON_SURPLUS_MODE_MAX> VictronSurplusModeWire("battery_charge");
uint8_t victron_grid_phases = 1;
unsigned long LastVictronMqttMillis = 0;

Action load_channels[kMaxRoutingActions];
int NbActions = 0;
uint8_t expert_regulation_mode = 0;
uint8_t regulation_gain = 1;
bool vacationEnabled = false;
uint32_t vacationEndEpoch = 0;
uint16_t maxRoutedW = 0;
bool siteCapActive = false;
uint32_t actionDailyCapWh[kMaxRoutingActions] = {};
bool actionCapHit[kMaxRoutingActions] = {};
bool mqttJsonCommands = false;
bool triacOffWhenSourceStale = false;
bool commissioningBlocksOutputs = false;
bool triacBackoffWhenHeaterIdle = false;
bool heaterLoadBackoffActive = false;
BalansunStr<32> loadProfileWire("water_heater_triac");

unsigned long startMillis;
unsigned long previousWifiMillis;
unsigned long previousHistoryMillis;
unsigned long previousWsMillis;
unsigned long previousWiMillis;
unsigned long last_metering_task_ms;
unsigned long previousTimer2sMillis;
unsigned long previousOverProdMillis;
unsigned long previousLEDsMillis;
unsigned long previousLoop;
unsigned long previousETX;
unsigned long poll_period_ms = 1000;
float previousLoopMin = 1000;
float previousLoopMax = 0;
float previousLoopMoy = 0;
unsigned long last_metering_task_at_ms;
float metering_task_ms_min = 1000;
float metering_task_ms_max = 0;
float metering_task_ms_avg = 0;
unsigned long previousMqttMillis;

float triac_delay_percent_f = 100;

WebServer server(80);

const char *ntpServer1 = "fr.pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
BalansunStr<BALANSUN_TIME_TZ_MAX> TimeTz = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";
BalansunStr<BALANSUN_TIME_NTP_MAX> TimeNtp1 = "fr.pool.ntp.org";
BalansunStr<BALANSUN_TIME_NTP_MAX> TimeNtp2 = "time.nist.gov";
BalansunStr<BALANSUN_SYNC_CLOCK_STR_MAX> sync_clock_str;
BalansunStr<BALANSUN_DATE_DDMMYYYY_MAX> currentDateStr;
bool time_sync_valid = false;
int wall_clock_decihours = 0;
int idxPromDuJour = 0;

float temperature = -127;
int triacOverrideMaxTempC = 70;

#if BALANSUN_REMOTE_DEBUG
RemoteDebug Debug;
#else
RmsDebugStub Debug;
#endif
WiFiClient MqttClient;
PubSubClient clientMQTT(MqttClient);
int WIFIbug = 0;
int meterPeerFailures = 0;

TaskHandle_t Task1;
