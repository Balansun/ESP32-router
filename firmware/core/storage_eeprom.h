#pragma once
#include <Arduino.h>

void eepromInit(void);
void eepromClearConsumptionHistory(void);
void eepromLoadMorningDayEnergy(void);
void balansun_on_clock_tick(void);
void loadConfigFromEeprom(void);
unsigned long eepromReadLayoutKey(void);
/** Reset in-RAM persisted settings to firmware factory defaults (before persistConfigToEeprom). */
void balansun_config_apply_factory_defaults(void);
int persistConfigToEeprom(void);

/** True when the most recent persist could not fit all settings in EEPROM capacity. */
extern bool RomPersistTruncated;
