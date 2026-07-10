#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_VictronGx
#include "victron_gx_meter.h"

#include "victron_gx_mqtt.h"

void VictronGxMeter::setup() { victron_gx_mqtt_begin(); }

void VictronGxMeter::appendDiagnostics(JsonObject doc, int linky_tail_max) {
  victron_gx_mqtt_append_diagnostics(doc, linky_tail_max);
}

IMeterDriver *balansun_meter_instance_victron_gx() {
  static VictronGxMeter instance;
  return &instance;
}

#else /* !BALANSUN_ENABLE_SOURCE_VictronGx */

#include "victron_gx_meter.h"

IMeterDriver *balansun_meter_instance_victron_gx() { return nullptr; }

#endif
