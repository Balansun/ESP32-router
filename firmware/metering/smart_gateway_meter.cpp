/*
 * smart_gateway_meter.cpp — Source SmartG: HTTP JSON from Smart Gateways device.
 * See: /en/hardware-pinout/ — source_smartg; GUIDE A.6.
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_SMARTG
#include "smart_gateway_meter.h"

#include "balansun_globals.h"
#include "json_flat_meter_logic.h"

bool SmartGatewayMeter::parseBody(const std::string &inner_json, JsonFlatMeterReading &out) {
  return json_flat_meter_logic_parse_smartg(inner_json, out);
}

void SmartGatewayMeter::storeRawBody(const std::string &inner_json) {
  SG_rawData = String(inner_json.c_str());
}

void SmartGatewayMeter::appendDiagnostics(JsonObject doc, int linky_tail_max) {
  (void)linky_tail_max;
  JsonObject sg = doc["smartg"].to<JsonObject>();
  sg["raw"] = SG_rawData;
}

IMeterDriver *balansun_meter_instance_smartg() {
  static SmartGatewayMeter instance;
  return &instance;
}

#endif /* BALANSUN_ENABLE_SOURCE_SMARTG */
