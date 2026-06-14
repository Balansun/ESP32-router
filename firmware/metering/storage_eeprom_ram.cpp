#include "storage_eeprom_ram.h"

#include <cstring>

RamEepromBackend::RamEepromBackend(int size) : buf_(static_cast<size_t>(size), 0xFF) {}

int RamEepromBackend::capacity() const { return static_cast<int>(buf_.size()); }

void RamEepromBackend::clear() { std::fill(buf_.begin(), buf_.end(), 0xFF); }

uint8_t RamEepromBackend::readByte(int address) {
  if (address < 0 || address >= capacity()) return 0xFF;
  return buf_[static_cast<size_t>(address)];
}

void RamEepromBackend::writeByte(int address, uint8_t value) {
  if (address < 0 || address >= capacity()) return;
  buf_[static_cast<size_t>(address)] = value;
}

uint16_t RamEepromBackend::readUShort(int address) {
  if (address < 0 || address + 1 >= capacity()) return 0xFFFF;
  return static_cast<uint16_t>(readByte(address) | (readByte(address + 1) << 8));
}

void RamEepromBackend::writeUShort(int address, uint16_t value) {
  writeByte(address, static_cast<uint8_t>(value & 0xFF));
  writeByte(address + 1, static_cast<uint8_t>((value >> 8) & 0xFF));
}

uint32_t RamEepromBackend::readULong(int address) {
  if (address < 0 || address + 3 >= capacity()) return 0xFFFFFFFFu;
  uint32_t v = readByte(address);
  v |= static_cast<uint32_t>(readByte(address + 1)) << 8;
  v |= static_cast<uint32_t>(readByte(address + 2)) << 16;
  v |= static_cast<uint32_t>(readByte(address + 3)) << 24;
  return v;
}

void RamEepromBackend::writeULong(int address, uint32_t value) {
  writeByte(address, static_cast<uint8_t>(value & 0xFF));
  writeByte(address + 1, static_cast<uint8_t>((value >> 8) & 0xFF));
  writeByte(address + 2, static_cast<uint8_t>((value >> 16) & 0xFF));
  writeByte(address + 3, static_cast<uint8_t>((value >> 24) & 0xFF));
}

std::string RamEepromBackend::readString(int address) {
  if (address < 0 || address >= capacity()) return "";
  std::string s;
  for (int i = address; i < capacity(); i++) {
    uint8_t c = readByte(i);
    if (c == 0) break;
    s.push_back(static_cast<char>(c));
  }
  return s;
}

void RamEepromBackend::writeString(int address, const std::string &value) {
  for (size_t i = 0; i <= value.size(); i++) {
    const int a = address + static_cast<int>(i);
    if (a >= capacity()) return;
    writeByte(a, i < value.size() ? static_cast<uint8_t>(value[i]) : 0);
  }
}
