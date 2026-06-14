#pragma once

#include "balansun_meter_driver_base.h"

class SerialMeterDriver : public MeterDriverBase {
 public:
  void poll() override;

 protected:
  virtual bool pollTransport() = 0;
};
