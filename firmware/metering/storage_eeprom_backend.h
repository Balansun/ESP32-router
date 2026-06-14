#pragma once

#include <cstdint>
#include <string>

/** Abstract EEPROM access for layout/extension logic (Arduino EEPROM or RAM mock). */
class IEepromBackend {
public:
  virtual ~IEepromBackend() = default;
  virtual int capacity() const = 0;
  virtual uint8_t readByte(int address) = 0;
  virtual void writeByte(int address, uint8_t value) = 0;
  virtual uint16_t readUShort(int address) = 0;
  virtual void writeUShort(int address, uint16_t value) = 0;
  virtual uint32_t readULong(int address) = 0;
  virtual void writeULong(int address, uint32_t value) = 0;
  virtual std::string readString(int address) = 0;
  virtual void writeString(int address, const std::string &value) = 0;
};
