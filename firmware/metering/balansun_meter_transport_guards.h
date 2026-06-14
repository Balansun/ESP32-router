#pragma once

#include "balansun_meter_sources_enable.h"

/* Shared metering modules linked only when a compiled source needs them. */

#define BALANSUN_METER_NEEDS_JSON_FLAT                                        \
  (BALANSUN_ENABLE_SOURCE_SHELLY_EM || BALANSUN_ENABLE_SOURCE_SHELLY_PRO ||      \
   BALANSUN_ENABLE_SOURCE_SMARTG || BALANSUN_ENABLE_SOURCE_HOMEW ||              \
   BALANSUN_ENABLE_SOURCE_ENPHASE)

#define BALANSUN_METER_NEEDS_LAN_HTTP                                         \
  (BALANSUN_METER_NEEDS_JSON_FLAT || BALANSUN_ENABLE_SOURCE_EXT ||              \
   BALANSUN_ENABLE_SOURCE_LINKY)

#define BALANSUN_METER_NEEDS_HTTP_RESPONSE (BALANSUN_METER_NEEDS_LAN_HTTP)
