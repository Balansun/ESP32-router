#include "balansun_regulation_persist.h"

#include "storage_eeprom_layout.h"

#include <algorithm>

namespace {

constexpr uint8_t kRegulationPersistVersion = 1;

}  // namespace

int balansun_regulation_persist_read(int address, IEepromBackend &eeprom, EepromExtensionFields &fields) {
  const int cap = eeprom.capacity();
  if (address < 0 || address + 2 > cap) return address;
  if (eeprom.readUShort(address) != kEepromRegulationV2Magic) {
    fields.regulationPersistPresent = false;
    return address;
  }
  address += static_cast<int>(sizeof(uint16_t));
  fields.regulationPersistPresent = true;
  if (address < cap) {
    (void)eeprom.readByte(address++);
  }
  if (address < cap) fields.expertRegulationMode = eeprom.readByte(address++);
  if (address < cap) fields.regulationGain = eeprom.readByte(address++);
  if (address < cap) {
    (void)eeprom.readByte(address++);
  }
  for (int i = 0; i < EepromExtensionFields::kRegCoeffsMax && address + 4 <= cap; i++) {
    fields.actionRegCoeffs[i].kp = eeprom.readByte(address++);
    fields.actionRegCoeffs[i].ki = eeprom.readByte(address++);
    fields.actionRegCoeffs[i].kd = eeprom.readByte(address++);
    fields.actionRegCoeffs[i].pid = eeprom.readByte(address++) != 0;
  }
  if (fields.regulationGain < 1) fields.regulationGain = 1;
  return address;
}

int balansun_regulation_persist_write(int address, IEepromBackend &eeprom, const EepromExtensionFields &fields) {
  const int cap = eeprom.capacity();
  if (address < 0 || address + 2 > cap) return address;
  eeprom.writeUShort(address, kEepromRegulationV2Magic);
  address += static_cast<int>(sizeof(uint16_t));
  if (address < cap) eeprom.writeByte(address++, kRegulationPersistVersion);
  if (address < cap) eeprom.writeByte(address++, fields.expertRegulationMode);
  if (address < cap) eeprom.writeByte(address++, fields.regulationGain);
  if (address < cap) eeprom.writeByte(address++, 1);
  for (int i = 0; i < EepromExtensionFields::kRegCoeffsMax && address < cap; i++) {
    eeprom.writeByte(address++, fields.actionRegCoeffs[i].kp);
    eeprom.writeByte(address++, fields.actionRegCoeffs[i].ki);
    eeprom.writeByte(address++, fields.actionRegCoeffs[i].kd);
    eeprom.writeByte(address++, fields.actionRegCoeffs[i].pid ? 1 : 0);
  }
  return address;
}
