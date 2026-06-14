#include "balansun_meter_driver_base.h"

#include "balansun_globals.h"
#include "balansun_runtime.h"

#include <esp_task_wdt.h>

void MeterDriverBase::markPollSuccess() {
  meter_reading_valid = true;
  esp_task_wdt_reset();
  if (cptLEDyellow > 30) {
    cptLEDyellow = 4;
  }
}

void MeterDriverBase::markHttpFailure() {
  delay(200);
  meterPeerFailures++;
}

void MeterDriverBase::applyFlatReading(const JsonFlatMeterReading &rd) {
  house_active_import_w = rd.active_import_w;
  house_active_export_w = rd.active_export_w;
  house_energy_import_wh = static_cast<int>(rd.energy_import_wh);
  house_energy_export_wh = static_cast<int>(rd.energy_export_wh);
  if (rd.apparent_import_va != 0 || rd.apparent_export_va != 0) {
    house_apparent_import_va = rd.apparent_import_va;
    house_apparent_export_va = rd.apparent_export_va;
  }
  onFlatReadingApplied(rd);
}

bool MeterDriverBase::applySnapshotFields(const MeterSnapshotFields &fields) {
  RmsRuntime &rt = balansun_runtime();
  rt.sync_from_globals();
  std::string err;
  if (!balansun_meter_logic_apply_fields(rt, fields, &err)) {
    return false;
  }
  rt.sync_to_globals();
  return true;
}
