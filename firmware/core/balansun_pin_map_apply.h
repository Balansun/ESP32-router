#pragma once

#include <ArduinoJson.h>
#include <WString.h>

bool balansun_pins_apply_config_json_fields(JsonObjectConst root, String &err);

/** Append hardware GPIO pin map fields to a JSON object (config export subset). */
void balansun_pins_append_json(JsonObject o);
