#pragma once

#include "balansun_lan_http_custom_meter.h"

class ShellyProMeter : public LanHttpCustomMeter {
 public:
  SourceId id() const override { return SourceId::ShellyPro; }
  const char *wire() const override { return "ShellyPro"; }
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
  String device_name_;
  String device_profile_;
  bool device_info_loaded_ = false;
};

IMeterDriver *balansun_meter_instance_shelly_pro();
