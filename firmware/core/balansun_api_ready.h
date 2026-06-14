#pragma once

#include "api_util.h"

/** False during balansun_setup() until setup completes (once per boot). */
extern bool balansun_boot_complete;

void balansun_boot_mark_started(void);
void balansun_boot_mark_complete(void);

/** True when telemetry GET routes may return 200. */
bool balansun_api_telemetry_ready(void);

const char *balansun_api_telemetry_not_ready_lifecycle_wire(void);

#define API_TELEMETRY_READY_GUARD() \
  do { \
    if (!balansun_api_telemetry_ready()) { \
      api_error_telemetry_not_ready(server); \
      return; \
    } \
  } while (0)
