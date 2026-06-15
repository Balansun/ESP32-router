#include "ds18b20_temperature_sensor.h"

#include <balansun/ds18_poll.h>

#ifndef BALANSUN_TARGET_ESP8266
#include <esp_task_wdt.h>
#endif

namespace {

void balansun_wdt_reset() {
#ifndef BALANSUN_TARGET_ESP8266
  esp_task_wdt_reset();
#endif
}

}  // namespace

void Ds18b20TemperatureSensor::destroyBus() {
  delete sensors_;
  delete one_wire_;
  sensors_ = nullptr;
  one_wire_ = nullptr;
  bus_pin_ = -1;
  bus_active_ = false;
  ds18_poll_ = {};
  cached_device_count_ = -1;
  last_device_scan_ms_ = 0;
}

bool Ds18b20TemperatureSensor::setup(int gpio) {
  if (gpio < 0) {
    invalidate();
    return false;
  }
  ensureBus(gpio);
  return bus_active_;
}

void Ds18b20TemperatureSensor::invalidate() { destroyBus(); }

void Ds18b20TemperatureSensor::service(uint32_t now_ms) { (void)now_ms; }

void Ds18b20TemperatureSensor::ensureBus(int gpio) {
  if (gpio < 0) {
    destroyBus();
    return;
  }
  if (one_wire_ && bus_pin_ == gpio) {
    bus_active_ = true;
    return;
  }
  destroyBus();
  bus_pin_ = gpio;
  one_wire_ = new OneWire(gpio);
  sensors_ = new DallasTemperature(one_wire_);
  sensors_->begin();
  sensors_->setWaitForConversion(false);
  one_wire_->reset();
  cached_device_count_ = sensors_->getDeviceCount();
  last_device_scan_ms_ = millis();
  if (cached_device_count_ > 0) {
    sensors_->requestTemperatures();
    ds18_poll_begin_request(ds18_poll_, last_device_scan_ms_);
  }
  bus_active_ = true;
}

bool Ds18b20TemperatureSensor::pollBusReadings(TemperatureReading out[], int max_out, int &discovered) {
  discovered = 0;
  if (!sensors_ || max_out <= 0) return false;
  balansun_wdt_reset();
  const uint32_t now = millis();
  constexpr uint32_t kDeviceScanIntervalMs = 30000;
  constexpr uint32_t kDeviceScanIdleMs = 5000;

  if (ds18_poll_.pending) {
    if (!ds18_poll_conversion_ready(ds18_poll_, now)) {
      return false;
    }
    const int count = cached_device_count_ > 0 ? cached_device_count_ : 0;
    discovered = count > max_out ? max_out : count;
    DeviceAddress addr;
    for (int i = 0; i < discovered; i++) {
      const float c = sensors_->getTempCByIndex(i);
      out[i].c = c;
      out[i].valid = isValidC(c);
      out[i].address = 0;
      if (sensors_->getAddress(addr, i)) {
        uint64_t packed = 0;
        for (int b = 0; b < 8; b++) {
          packed = (packed << 8) | addr[b];
        }
        out[i].address = packed;
      }
    }
    ds18_poll_mark_read(ds18_poll_);
    balansun_wdt_reset();
  }

  const uint32_t scan_interval =
      (cached_device_count_ <= 0) ? kDeviceScanIdleMs : kDeviceScanIntervalMs;
  if (cached_device_count_ < 0 || (now - last_device_scan_ms_) >= scan_interval) {
    if (one_wire_) one_wire_->reset();
    const int prev_count = cached_device_count_;
    cached_device_count_ = sensors_->getDeviceCount();
    last_device_scan_ms_ = now;
    balansun_wdt_reset();
    if (cached_device_count_ > 0 && prev_count <= 0 && !ds18_poll_.pending) {
      sensors_->requestTemperatures();
      ds18_poll_begin_request(ds18_poll_, now);
      return false;
    }
  }
  if (cached_device_count_ <= 0) {
    return discovered > 0;
  }
  if (!ds18_poll_.pending) {
    sensors_->requestTemperatures();
    ds18_poll_begin_request(ds18_poll_, now);
  }
  return discovered > 0;
}
