/*
 * homewizard_meter.cpp — Source HomeW: HTTP API HomeWizard P1 / energy socket.
 * See: /en/hardware-pinout/ §17 recap; GUIDE A.6.
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_HOMEW
#include "homewizard_meter.h"

#include "balansun_globals.h"
#include "json_flat_meter_logic.h"

bool HomeWizardMeter::parseBody(const std::string &inner_json, JsonFlatMeterReading &out) {
  return json_flat_meter_logic_parse_homewizard(inner_json, out);
}

void HomeWizardMeter::storeRawBody(const std::string &inner_json) {
  HW_rawData = String(inner_json.c_str());
}

void HomeWizardMeter::onFlatReadingApplied(const JsonFlatMeterReading &rd) {
  (void)rd;
  house_voltage_v = 230.0f;
  house_power_factor = 1.0f;
}

void HomeWizardMeter::appendDiagnostics(JsonObject doc, int linky_tail_max) {
  (void)linky_tail_max;
  JsonObject hw = doc["homewizard"].to<JsonObject>();
  hw["raw"] = HW_rawData;
}

IMeterDriver *balansun_meter_instance_homew() {
  static HomeWizardMeter instance;
  return &instance;
}

#endif /* BALANSUN_ENABLE_SOURCE_HOMEW */
