/*
 * storage_eeprom_layout.cpp — Compile-time checks for fixed EEPROM regions (no I/O).
 * See: storage_eeprom_layout.h for the full address map and extension magics.
 */
#include "storage_eeprom_layout.h"

#include "balansun_board.h"

bool storage_eeprom_layout_validate() {
  const int histoBytes = kEepromNbJour * 4;
  if (kEepromAdrHistoAn + histoBytes > kEepromAdrTriacImportJ0) return false;
  if (kEepromAdrEMinjecte0 + 4 > kEepromAdrcurrentDateStr) return false;
  if (kEepromAdrLastStockConso + 2 > kEepromAdrParaActions) return false;
  if (kEepromAdrParaActions >= kEepromSize) return false;
  if (kEepromSize > 4096) return false;
  return true;
}

uint32_t storage_eeprom_expected_cle_rom_init() { return (uint32_t)kEepromLayoutInit; }
