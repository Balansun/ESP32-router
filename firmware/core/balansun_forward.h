#pragma once

#include <Arduino.h>

/* storage_eeprom.cpp */
void eepromInit(void);
void eepromClearConsumptionHistory(void);
void eepromLoadMorningDayEnergy(void);
void balansun_on_clock_tick(void);
String eepromFormatYearlyEnergyHistory(void);
int eepromHistoryDaysCapacity(void);
int eepromHistoryDaysRetained(void);
bool eepromHistoryReferenceDateIso(String &isoOut);
bool eepromHistoryReadDailyMetrics(
    int logicalDayIdx,
    long &ch1ImportWh,
    long &ch1ExportWh,
    long &ch2ImportWh,
    long &ch2ExportWh);
bool eepromHistoryWriteDailyMetrics(
    int logicalDayIdx,
    long ch1ImportWh,
    long ch1ExportWh,
    long ch2ImportWh,
    long ch2ExportWh);
bool eepromHistoryImportDailyMetrics(
    const long ch1ImportWh[],
    const long ch1ExportWh[],
    const long ch2ImportWh[],
    const long ch2ExportWh[],
    int count,
    const char *latestDateIso,
    String &err);
/** Flush staged daily ring to NVS when pending (also called from balansun_loop). */
void eepromHistoryServicePendingCommit(void);
bool eepromHistoryHasPendingCommit(void);
unsigned long eepromReadLayoutKey(void);
void loadConfigFromEeprom(void);
void balansun_config_apply_factory_defaults(void);
int persistConfigToEeprom(void);
void Calibration(int address);

/* http_server.cpp */
void Init_Server(void);
void balansun_http_invalidate_binding(void);
void balansun_http_ensure_listening(void);

/* mqtt_ha.cpp */
void publishMqttLoop(void);
void pmqtt_mqtt_service_tick(void);

/* Source drivers */
void jsy_mk194t_setup(void);
void jsy_mk194t_poll(void);
void jsy_mk333_setup(void);
void jsy_mk333_send_request(void);
void jsy_mk333_poll(void);
void uxi_probe_setup(void);
void uxi_probe_poll(void);
void linky_meter_setup(void);
void linky_meter_poll(void);
void enphase_envoy_setup(void);
void enphase_envoy_poll(void);
void smart_gateway_poll(void);
void shelly_em_poll(void);
void external_peer_poll(void);
