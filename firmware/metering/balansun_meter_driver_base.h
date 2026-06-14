#pragma once

#include "balansun_meter_driver_iface.h"
#include "balansun_meter_logic.h"
#include "json_flat_meter_logic.h"

#include <Arduino.h>

class MeterDriverBase : public IMeterDriver {
 protected:
  void markPollSuccess();
  void markHttpFailure();
  void applyFlatReading(const JsonFlatMeterReading &rd);
  bool applySnapshotFields(const MeterSnapshotFields &fields);
  virtual void onFlatReadingApplied(const JsonFlatMeterReading &rd) { (void)rd; }
};
