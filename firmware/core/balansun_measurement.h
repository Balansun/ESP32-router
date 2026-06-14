#pragma once

#include "balansun_pub.h"

/** Stable alias for published meter readings (same layout as BalansunPublic). */
using MeasurementSnapshot = BalansunPublic;

/** Last coherent snapshot after `BalansunPublishFromGlobals()` (core-0 task). */
extern MeasurementSnapshot g_lastRmsMeasurement;

void balansun_measurement_refresh_last();
