#pragma once

#include "storage_eeprom_backend.h"

#include <cstdint>
#include <vector>

/** In-memory EEPROM for host-native tests. */
class RamEepromBackend : public IEepromBackend {
public:
  explicit RamEepromBackend(int size = 4090);
  int capacity() const override;
  uint8_t readByte(int address) override;
  void writeByte(int address, uint8_t value) override;
  uint16_t readUShort(int address) override;
  void writeUShort(int address, uint16_t value) override;
  uint32_t readULong(int address) override;
  void writeULong(int address, uint32_t value) override;
  std::string readString(int address) override;
  void writeString(int address, const std::string &value) override;
  void clear();

private:
  std::vector<uint8_t> buf_;
};
