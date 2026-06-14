#pragma once

#include "balansun_meter_driver_base.h"

class BenchSimMeter : public MeterDriverBase {
 public:
  SourceId id() const override { return SourceId::NotDef; }
  const char *wire() const override { return "NotDef"; }
  void poll() override;
  uint16_t basePeriodMs() const override { return 600; }
};

IMeterDriver *balansun_meter_instance_notdef();
