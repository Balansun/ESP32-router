#pragma once

#include "storage_eeprom_backend.h"
#include "storage_eeprom_extension.h"

/** Read/write regulation block (magic 0xE228) at extension tail. */
int balansun_regulation_persist_read(int address, IEepromBackend &eeprom, EepromExtensionFields &fields);
int balansun_regulation_persist_write(int address, IEepromBackend &eeprom, const EepromExtensionFields &fields);
