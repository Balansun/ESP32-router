#pragma once

#include <balansun/temperature_reading.h>

#include <cstdint>

class ITemperatureSensor {
 public:
  virtual ~ITemperatureSensor() = default;
  virtual const char *kind() const = 0;
  virtual bool setup(int gpio) = 0;
  virtual void invalidate() = 0;
  virtual void service(uint32_t now_ms) = 0;
  virtual bool pollBusReadings(TemperatureReading out[], int max_out, int &discovered) = 0;
};

class TemperatureSensorBase : public ITemperatureSensor {
 protected:
  static bool isValidC(float c);
};
