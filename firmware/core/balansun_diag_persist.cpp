#include "balansun_diag_persist.h"

#include "storage_eeprom_layout.h"


int balansun_diag_persist_read(int address, IEepromBackend &eeprom, SelfTestPersisted &self_test,
                          TriacCalibrationTable &triac_cal) {
  const int cap = eeprom.capacity();
  if (address < 0 || address + 2 > cap) return address;
  if (eeprom.readUShort(address) != kEepromDiagMagic) return address;
  address += static_cast<int>(sizeof(uint16_t));
  if (address + 12 > cap) return address;
  const uint8_t flags = eeprom.readByte(address);
  self_test.pending = (flags & 0x01) != 0;
  self_test.skipped = (flags & 0x02) != 0;
  self_test.zc_ok = (flags & 0x04) != 0;
  self_test.triac_ok = (flags & 0x08) != 0;
  self_test.source_ok = (flags & 0x10) != 0;
  address += 1;
  self_test.run_epoch = eeprom.readULong(address);
  address += static_cast<int>(sizeof(uint32_t));
  self_test.zc_edges_per_sec = eeprom.readUShort(address);
  address += static_cast<int>(sizeof(uint16_t));
  address += 2;
  if (address >= cap) return address;
  triac_cal.enabled = eeprom.readByte(address) != 0;
  address += 1;
  for (int i = 0; i < 3; i++) {
    if (address + 3 > cap) break;
    triac_cal.points[i].duty_pct = eeprom.readByte(address);
    address += 1;
    triac_cal.points[i].measured_w = eeprom.readUShort(address);
    address += static_cast<int>(sizeof(uint16_t));
  }
  return address;
}

int balansun_diag_persist_write(int address, IEepromBackend &eeprom, const SelfTestPersisted &self_test,
                           const TriacCalibrationTable &triac_cal) {
  const int cap = eeprom.capacity();
  if (address < 0 || address + 2 > cap) return address;
  eeprom.writeUShort(address, kEepromDiagMagic);
  address += static_cast<int>(sizeof(uint16_t));
  uint8_t flags = 0;
  if (self_test.pending) flags |= 0x01;
  if (self_test.skipped) flags |= 0x02;
  if (self_test.zc_ok) flags |= 0x04;
  if (self_test.triac_ok) flags |= 0x08;
  if (self_test.source_ok) flags |= 0x10;
  eeprom.writeByte(address, flags);
  address += 1;
  eeprom.writeULong(address, self_test.run_epoch);
  address += static_cast<int>(sizeof(uint32_t));
  eeprom.writeUShort(address, self_test.zc_edges_per_sec);
  address += static_cast<int>(sizeof(uint16_t));
  address += 2;
  eeprom.writeByte(address, triac_cal.enabled ? 1 : 0);
  address += 1;
  for (int i = 0; i < 3; i++) {
    eeprom.writeByte(address, triac_cal.points[i].duty_pct);
    address += 1;
    eeprom.writeUShort(address, triac_cal.points[i].measured_w);
    address += static_cast<int>(sizeof(uint16_t));
  }
  return address;
}
