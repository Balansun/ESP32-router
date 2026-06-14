#pragma once

#include "balansun_lan_http_json_meter.h"

class SmartGatewayMeter : public LanHttpJsonMeter {
 public:
  SourceId id() const override { return SourceId::SmartG; }
  const char *wire() const override { return "SmartG"; }
  uint16_t basePeriodMs() const override { return 300; }
  uint8_t flags() const override {
    return BALANSUN_METER_RSF_POLL_BACKOFF | BALANSUN_METER_RSF_TOUCH_LAST_MS;
  }
  void appendDiagnostics(JsonObject doc, int linky_tail_max) override;

 protected:
  uint16_t httpPort() const override { return 82; }
  const char *httpPath() const override { return "/smartmeter/api/read"; }
  bool parseBody(const std::string &inner_json, JsonFlatMeterReading &out) override;
  void storeRawBody(const std::string &inner_json) override;
};

IMeterDriver *balansun_meter_instance_smartg();
