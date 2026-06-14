#pragma once

#include "storage_eeprom_backend.h"

/** Production EEPROM backend wrapping Arduino EEPROM API. */
class ArduinoEepromBackend : public IEepromBackend {
public:
  int capacity() const override;
  uint8_t readByte(int address) override;
  void writeByte(int address, uint8_t value) override;
  uint16_t readUShort(int address) override;
  void writeUShort(int address, uint16_t value) override;
  uint32_t readULong(int address) override;
  void writeULong(int address, uint32_t value) override;
  std::string readString(int address) override;
  void writeString(int address, const std::string &value) override;
};

ArduinoEepromBackend &storage_eeprom_arduino_backend();
