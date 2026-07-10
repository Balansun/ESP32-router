#pragma once

#include "balansun_event_driven_meter.h"

class VictronGxMeter : public EventDrivenMeter {
 public:
  SourceId id() const override { return SourceId::VictronGx; }
  const char *wire() const override { return "VictronGx"; }
  uint16_t basePeriodMs() const override { return 600; }
  uint8_t flags() const override { return BALANSUN_METER_RSF_TOUCH_LAST_MS; }
  void setup() override;
  void appendDiagnostics(JsonObject doc, int linky_tail_max) override;
};

IMeterDriver *balansun_meter_instance_victron_gx();
