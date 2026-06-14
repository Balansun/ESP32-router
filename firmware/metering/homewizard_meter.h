#pragma once

#include "balansun_lan_http_json_meter.h"

class HomeWizardMeter : public LanHttpJsonMeter {
 public:
  SourceId id() const override { return SourceId::HomeW; }
  const char *wire() const override { return "HomeW"; }
  uint16_t basePeriodMs() const override { return 300; }
  uint8_t flags() const override {
    return BALANSUN_METER_RSF_POLL_BACKOFF | BALANSUN_METER_RSF_TOUCH_LAST_MS;
  }
  void appendDiagnostics(JsonObject doc, int linky_tail_max) override;

 protected:
  const char *httpPath() const override { return "/api/v1/data"; }
  bool parseBody(const std::string &inner_json, JsonFlatMeterReading &out) override;
  void storeRawBody(const std::string &inner_json) override;
  void onFlatReadingApplied(const JsonFlatMeterReading &rd) override;
};

IMeterDriver *balansun_meter_instance_homew();
