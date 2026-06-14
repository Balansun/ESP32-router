#pragma once

#include <balansun/ds18_poll.h>
#include <balansun/temperature_reading.h>
#include <balansun/temperature_sensor_iface.h>

#include <DallasTemperature.h>
#include <OneWire.h>

/** DS18B20 1-Wire bus driver (router + action node). */
class Ds18b20TemperatureSensor : public TemperatureSensorBase {
 public:
  const char *kind() const override { return "ds18b20"; }
  bool setup(int gpio) override;
  void invalidate() override;
  void service(uint32_t now_ms) override;
  bool pollBusReadings(TemperatureReading out[], int max_out, int &discovered) override;
  bool hasPendingConversion() const { return ds18_poll_.pending; }
  uint32_t conversionRequestedMs() const { return ds18_poll_.last_request_ms; }

 private:
  void destroyBus();
  void ensureBus(int gpio);

  OneWire *one_wire_ = nullptr;
  DallasTemperature *sensors_ = nullptr;
  int bus_pin_ = -1;
  Ds18PollState ds18_poll_{};
  int cached_device_count_ = -1;
  uint32_t last_device_scan_ms_ = 0;
  bool bus_active_ = false;
};
