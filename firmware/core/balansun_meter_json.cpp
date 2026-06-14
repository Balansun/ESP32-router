#include "balansun_meter_json.h"
#include "balansun_globals.h"
#include "balansun_meter_logic.h"
#include "balansun_pub.h"
#include "balansun_runtime.h"
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <string>

static void takeInt(JsonObject o, const char *key, int &dst, bool &any) {
  if (o[key].isNull()) return;
  dst = (int)o[key].as<long>();
  any = true;
}

static void takeLong(JsonObject o, const char *key, long &dst, bool &any) {
  if (o[key].isNull()) return;
  dst = o[key].as<long>();
  any = true;
}

static void takeFloat(JsonObject o, const char *key, float &dst, bool &any) {
  if (o[key].isNull()) return;
  dst = o[key].as<float>();
  any = true;
}

static void takeDouble(JsonObject o, const char *key, double &dst, bool &any) {
  if (o[key].isNull()) return;
  dst = o[key].as<double>();
  any = true;
}

static void applyHouseBlockToFields(JsonObject ch, bool second, MeterSnapshotFields &fields, bool &any) {
  if (!ch) return;
  MeterChannelState &dst = second ? fields.second : fields.house;
  takeInt(ch, "active_import_w", dst.active_import_w, any);
  takeInt(ch, "active_export_w", dst.active_export_w, any);
  takeInt(ch, "apparent_import_va", dst.apparent_import_va, any);
  takeInt(ch, "apparent_export_va", dst.apparent_export_va, any);
  takeDouble(ch, "energy_day_import_wh", dst.energy_day_import_wh, any);
  takeDouble(ch, "energy_day_export_wh", dst.energy_day_export_wh, any);
  takeDouble(ch, "energy_total_import_wh", dst.energy_total_import_wh, any);
  takeDouble(ch, "energy_total_export_wh", dst.energy_total_export_wh, any);
  if (second) {
    fields.has_second = true;
  } else {
    fields.has_house = true;
  }
}

bool ApplyMeterSnapshotFromJson(JsonObject root, String *errOut) {
  MeterSnapshotFields fields;
  bool any = false;
  if (!root["house"].isNull() && root["house"].is<JsonObject>()) {
    applyHouseBlockToFields(root["house"].as<JsonObject>(), false, fields, any);
  }
  if (!root["second"].isNull() && root["second"].is<JsonObject>()) {
    applyHouseBlockToFields(root["second"].as<JsonObject>(), true, fields, any);
  }
  if (!root["raw_meter"].isNull() && root["raw_meter"].is<JsonObject>()) {
    JsonObject rw = root["raw_meter"].as<JsonObject>();
    takeFloat(rw, "voltage_house_v", fields.raw.voltage_house_v, any);
    takeFloat(rw, "current_house_a", fields.raw.current_house_a, any);
    takeFloat(rw, "pf_house", fields.raw.pf_house, any);
    takeFloat(rw, "voltage_second_v", fields.raw.voltage_second_v, any);
    takeFloat(rw, "current_second_a", fields.raw.current_second_a, any);
    takeFloat(rw, "pf_second", fields.raw.pf_second, any);
    takeFloat(rw, "freq_hz", fields.raw.freq_hz, any);
    fields.has_raw = any;
  }
  if (!any) {
    if (errOut) *errOut = "no_meter_fields";
    return false;
  }
  RmsRuntime &rt = balansun_runtime();
  rt.sync_from_globals();
  std::string errStd;
  if (!balansun_meter_logic_apply_fields(rt, fields, &errStd)) {
    if (errOut) *errOut = String(errStd.c_str());
    return false;
  }
  rt.sync_to_globals();
  BalansunPublishFromGlobals();
  esp_task_wdt_reset();
  if (cptLEDyellow > 30) {
    cptLEDyellow = 4;
  }
  return true;
}
