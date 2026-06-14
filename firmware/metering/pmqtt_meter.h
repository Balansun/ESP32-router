#pragma once

#include "balansun_event_driven_meter.h"

class PmqttMeter : public EventDrivenMeter {
 public:
  SourceId id() const override { return SourceId::Pmqtt; }
  const char *wire() const override { return "Pmqtt"; }
  uint16_t basePeriodMs() const override { return 600; }
  uint8_t flags() const override { return BALANSUN_METER_RSF_TOUCH_LAST_MS; }
  void appendDiagnostics(JsonObject doc, int linky_tail_max) override;
};

IMeterDriver *balansun_meter_instance_pmqtt();
