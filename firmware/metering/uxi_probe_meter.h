#pragma once

#include "balansun_adc_meter_driver.h"

class AnalogMeter : public AdcMeterDriver {
 public:
  SourceId id() const override { return SourceId::Analog; }
  const char *wire() const override { return "Analog"; }
  void setup() override;
  void poll() override;
  uint16_t basePeriodMs() const override { return 40; }
  void appendDiagnostics(JsonObject doc, int linky_tail_max) override;
};

IMeterDriver *balansun_meter_instance_analog();
