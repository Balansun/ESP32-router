#pragma once

#include "balansun_serial_meter_driver.h"

class JsyMk194Meter : public SerialMeterDriver {
 public:
  SourceId id() const override { return SourceId::JsyMk194; }
  const char *wire() const override { return "JsyMk194"; }
  void setup() override;
  uint16_t basePeriodMs() const override { return 400; }
  void appendDiagnostics(JsonObject doc, int linky_tail_max) override;

  /** CH2 only — house globals stay with Cerbo/Victron surplus source. */
  bool pollTriacChannelOnly();

 protected:
  bool pollTransport() override;
};

IMeterDriver *balansun_meter_instance_jsy_mk194();
bool jsy_mk194t_poll_triac_channel(void);
