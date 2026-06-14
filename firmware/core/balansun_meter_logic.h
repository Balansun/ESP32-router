#pragma once

#include "balansun_runtime.h"

#include <string>

struct MeterSnapshotFields {
  bool has_house = false;
  bool has_second = false;
  bool has_raw = false;
  MeterChannelState house;
  MeterChannelState second;
  RawMeterState raw;
};

/** Apply parsed fields into runtime (no publish / WDT side effects). Returns false if empty. */
bool balansun_meter_logic_apply_fields(RmsRuntime &rt, const MeterSnapshotFields &fields, std::string *errOut = nullptr);
