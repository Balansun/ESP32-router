#include "balansun_measurement.h"

MeasurementSnapshot g_lastRmsMeasurement;

void balansun_measurement_refresh_last() {
  g_lastRmsMeasurement = BalansunReadSnapshot();
}
