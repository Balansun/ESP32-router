#pragma once

#include "storage_eeprom_backend.h"

struct SelfTestPersisted;
struct TriacCalibrationTable;

inline int balansun_diag_persist_read(int address, IEepromBackend &, SelfTestPersisted &,
                                 TriacCalibrationTable &) {
  return address;
}

inline int balansun_diag_persist_write(int address, IEepromBackend &, const SelfTestPersisted &,
                                  const TriacCalibrationTable &) {
  return address;
}
