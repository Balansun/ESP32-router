#pragma once

#include "balansun_tls_http_meter.h"

class EnphaseMeter : public TlsHttpMeter {
 public:
  SourceId id() const override { return SourceId::Enphase; }
  const char *wire() const override { return "Enphase"; }
  void setup() override;
  void poll() override;
  uint16_t basePeriodMs() const override { return 600; }
  uint8_t flags() const override {
    return BALANSUN_METER_RSF_POLL_BACKOFF | BALANSUN_METER_RSF_TOUCH_LAST_MS;
  }
  void appendDiagnostics(JsonObject doc, int linky_tail_max) override;
};

IMeterDriver *balansun_meter_instance_enphase();
