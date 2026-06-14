#pragma once

#include "balansun_serial_meter_driver.h"

class LinkyMeter : public SerialMeterDriver {
 public:
  SourceId id() const override { return SourceId::Linky; }
  const char *wire() const override { return "Linky"; }
  void setup() override;
  void poll() override;
  uint16_t basePeriodMs() const override { return 2; }
  void appendDiagnostics(JsonObject doc, int linky_tail_max) override;

 protected:
  bool pollTransport() override;
};

IMeterDriver *balansun_meter_instance_linky();
