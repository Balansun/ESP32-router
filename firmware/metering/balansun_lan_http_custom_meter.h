#pragma once

#include "balansun_meter_driver_base.h"

#include <Arduino.h>

class LanHttpCustomMeter : public MeterDriverBase {
 public:
  void poll() override;

 protected:
  virtual uint16_t httpPort() const { return 80; }
  virtual void buildRequest(String &host_out, String &path_out) = 0;
  virtual bool parseAndApply(const String &body) = 0;
};
