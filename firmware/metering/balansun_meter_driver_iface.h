#pragma once

/*
 * balansun_meter_driver_iface.h — Virtual meter driver interface.
 */

#include "balansun_source_types.h"

#include <ArduinoJson.h>
#include <cstdint>

enum BalansunMeterRegistryFlags : uint8_t {
  BALANSUN_METER_RSF_NONE = 0,
  BALANSUN_METER_RSF_POLL_BACKOFF = 1,
  BALANSUN_METER_RSF_TOUCH_LAST_MS = 2,
};

class IMeterDriver {
 public:
  virtual ~IMeterDriver() = default;
  virtual SourceId id() const = 0;
  virtual const char *wire() const = 0;
  virtual void setup() {}
  virtual void poll() = 0;
  virtual uint16_t basePeriodMs() const = 0;
  virtual uint8_t flags() const { return BALANSUN_METER_RSF_NONE; }
  virtual void appendDiagnostics(JsonObject doc, int linky_tail_max) { (void)doc; (void)linky_tail_max; }
};
