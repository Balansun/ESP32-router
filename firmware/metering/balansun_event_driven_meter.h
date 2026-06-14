#pragma once

#include "balansun_meter_driver_base.h"

#include <Arduino.h>

class EventDrivenMeter : public MeterDriverBase {
 public:
  void poll() override {}
};
