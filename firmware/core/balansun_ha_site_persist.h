#pragma once

#include "storage_eeprom_backend.h"
#include "storage_eeprom_extension.h"

/** Extension tail: vacation, site cap, per-action daily caps, mqtt_json_commands (magic 0xE239). */
int balansun_ha_site_persist_read(int address, IEepromBackend &eeprom, EepromExtensionFields &fields);
int balansun_ha_site_persist_write(int address, IEepromBackend &eeprom, const EepromExtensionFields &fields);
