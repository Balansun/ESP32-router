#pragma once

#include "balansun_serial_meter_driver.h"

class JsyMk333Meter : public SerialMeterDriver {
 public:
  SourceId id() const override { return SourceId::JsyMk333; }
  const char *wire() const override { return "JsyMk333"; }
  void setup() override;
  uint16_t basePeriodMs() const override { return 800; }
  void appendDiagnostics(JsonObject doc, int linky_tail_max) override;

 protected:
  bool pollTransport() override;
};

IMeterDriver *balansun_meter_instance_jsy_mk333();

/** Legacy hook: prime Serial2 before first poll cycle (balansun_source.cpp). */
void jsy_mk333_send_request(void);
