#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

/** Apply JSON shaped like /api/v1/measurements (house, second, optional raw_meter). */
bool ApplyMeterSnapshotFromJson(JsonObject root, String *errOut);
