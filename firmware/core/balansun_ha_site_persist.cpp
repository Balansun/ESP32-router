#include "balansun_ha_site_persist.h"

#include "storage_eeprom_layout.h"

namespace {
constexpr uint8_t kHaSiteVersion = 3;
}  // namespace

int balansun_ha_site_persist_read(int address, IEepromBackend &eeprom, EepromExtensionFields &fields) {
  const int cap = eeprom.capacity();
  if (address < 0 || address + 2 > cap) return address;
  if (eeprom.readUShort(address) != kEepromHaSiteMagic) {
    fields.haSitePersistPresent = false;
    return address;
  }
  address += static_cast<int>(sizeof(uint16_t));
  fields.haSitePersistPresent = true;
  uint8_t version = 0;
  if (address < cap) {
    version = eeprom.readByte(address++);
    if (version != 2 && version != kHaSiteVersion) {
      fields.haSitePersistPresent = false;
      return address;
    }
  }
  if (address < cap) fields.vacationEnabled = eeprom.readByte(address++) != 0;
  if (address + 4 <= cap) {
    fields.vacationEndEpoch = eeprom.readULong(address);
    address += static_cast<int>(sizeof(uint32_t));
  }
  if (address + 2 <= cap) {
    fields.maxRoutedW = eeprom.readUShort(address);
    address += static_cast<int>(sizeof(uint16_t));
  }
  if (address < cap) fields.mqttJsonCommands = eeprom.readByte(address++) != 0;
  if (address < cap) fields.triacOffWhenSourceStale = eeprom.readByte(address++) != 0;
  if (version >= 3 && address < cap) {
    fields.triacBackoffWhenHeaterIdle = eeprom.readByte(address++) != 0;
  }
  for (int i = 0; i < EepromExtensionFields::kRegCoeffsMax && address + 4 <= cap; i++) {
    fields.actionDailyCapWh[i] = eeprom.readULong(address);
    address += static_cast<int>(sizeof(uint32_t));
  }
  return address;
}

int balansun_ha_site_persist_write(int address, IEepromBackend &eeprom, const EepromExtensionFields &fields) {
  const int cap = eeprom.capacity();
  if (address < 0 || address + 2 > cap) return address;
  eeprom.writeUShort(address, kEepromHaSiteMagic);
  address += static_cast<int>(sizeof(uint16_t));
  if (address < cap) eeprom.writeByte(address++, kHaSiteVersion);
  if (address < cap) eeprom.writeByte(address++, fields.vacationEnabled ? 1 : 0);
  if (address + 4 <= cap) {
    eeprom.writeULong(address, fields.vacationEndEpoch);
    address += static_cast<int>(sizeof(uint32_t));
  }
  if (address + 2 <= cap) {
    eeprom.writeUShort(address, fields.maxRoutedW);
    address += static_cast<int>(sizeof(uint16_t));
  }
  if (address < cap) eeprom.writeByte(address++, fields.mqttJsonCommands ? 1 : 0);
  if (address < cap) eeprom.writeByte(address++, fields.triacOffWhenSourceStale ? 1 : 0);
  if (address < cap) eeprom.writeByte(address++, fields.triacBackoffWhenHeaterIdle ? 1 : 0);
  for (int i = 0; i < EepromExtensionFields::kRegCoeffsMax && address + 4 <= cap; i++) {
    eeprom.writeULong(address, fields.actionDailyCapWh[i]);
    address += static_cast<int>(sizeof(uint32_t));
  }
  return address;
}
