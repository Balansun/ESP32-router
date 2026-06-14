#pragma once

/*
 * balansun_meter_driver.h — Profile-filtered meter driver table rows (balansun_source.cpp).
 */

#include "balansun_source_types.h"

#include <cstdint>

struct BalansunMeterDriver {
  SourceId id;
  const char *wire;
  void (*setup_fn)(void);
  void (*poll_fn)(void);
  uint16_t base_period_ms;
  uint8_t flags;
};

#define BALANSUN_METER_ROW(id, wire, setup, poll, period, flags) \
  { id, wire, setup, poll, period, flags },
