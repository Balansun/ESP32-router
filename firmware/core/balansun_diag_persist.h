#pragma once

#include "balansun_self_test.h"
#include "balansun_triac_calibration_logic.h"
#include "storage_eeprom_backend.h"

int balansun_diag_persist_read(int address, IEepromBackend &eeprom, SelfTestPersisted &self_test,
                          TriacCalibrationTable &triac_cal);
int balansun_diag_persist_write(int address, IEepromBackend &eeprom, const SelfTestPersisted &self_test,
                           const TriacCalibrationTable &triac_cal);
