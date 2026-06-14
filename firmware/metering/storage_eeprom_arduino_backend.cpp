#include "storage_eeprom_arduino_backend.h"

#include "storage_eeprom_layout.h"

#include <Arduino.h>
#include <EEPROM.h>

int ArduinoEepromBackend::capacity() const { return kEepromSize; }

uint8_t ArduinoEepromBackend::readByte(int address) { return EEPROM.read(address); }

void ArduinoEepromBackend::writeByte(int address, uint8_t value) { EEPROM.write(address, value); }

uint16_t ArduinoEepromBackend::readUShort(int address) { return EEPROM.readUShort(address); }

void ArduinoEepromBackend::writeUShort(int address, uint16_t value) { EEPROM.writeUShort(address, value); }

uint32_t ArduinoEepromBackend::readULong(int address) { return EEPROM.readULong(address); }

void ArduinoEepromBackend::writeULong(int address, uint32_t value) { EEPROM.writeULong(address, value); }

std::string ArduinoEepromBackend::readString(int address) {
  return std::string(EEPROM.readString(address).c_str());
}

void ArduinoEepromBackend::writeString(int address, const std::string &value) {
  EEPROM.writeString(address, String(value.c_str()));
}

static ArduinoEepromBackend g_arduino_eeprom_backend;

ArduinoEepromBackend &storage_eeprom_arduino_backend() { return g_arduino_eeprom_backend; }
