#pragma once

#include "balansun_meter_driver_base.h"

#include <string>

class LanHttpJsonMeter : public MeterDriverBase {
 public:
  void poll() override;

 protected:
  virtual uint16_t httpPort() const { return 80; }
  virtual const char *httpPath() const = 0;
  virtual bool parseBody(const std::string &inner_json, JsonFlatMeterReading &out) = 0;
  virtual void storeRawBody(const std::string &inner_json) { (void)inner_json; }
};
