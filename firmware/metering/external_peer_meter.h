#pragma once

#include "balansun_lan_http_custom_meter.h"

class ExternalPeerMeter : public LanHttpCustomMeter {
 public:
  SourceId id() const override { return SourceId::BalansunPeer; }
  const char *wire() const override { return "BalansunPeer"; }
  uint16_t basePeriodMs() const override { return 800; }
  uint8_t flags() const override {
    return BALANSUN_METER_RSF_POLL_BACKOFF | BALANSUN_METER_RSF_TOUCH_LAST_MS;
  }
  void appendDiagnostics(JsonObject doc, int linky_tail_max) override;
  void poll() override;

 protected:
  void buildRequest(String &host_out, String &path_out) override;
  bool parseAndApply(const String &body) override;
};

IMeterDriver *balansun_meter_instance_balansun_peer();
