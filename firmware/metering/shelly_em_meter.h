#pragma once

#include "balansun_lan_http_custom_meter.h"

class ShellyEmMeter : public LanHttpCustomMeter {
 public:
  SourceId id() const override { return SourceId::ShellyEm; }
  const char *wire() const override { return "ShellyEm"; }
  uint16_t basePeriodMs() const override { return 300; }
  uint8_t flags() const override {
    return BALANSUN_METER_RSF_POLL_BACKOFF | BALANSUN_METER_RSF_TOUCH_LAST_MS;
  }
  void poll() override;
  void appendDiagnostics(JsonObject doc, int linky_tail_max) override;

 protected:
  void buildRequest(String &host_out, String &path_out) override;
  bool parseAndApply(const String &body) override;

 private:
  int polled_channel_ = 0;
};

IMeterDriver *balansun_meter_instance_shelly_em();
