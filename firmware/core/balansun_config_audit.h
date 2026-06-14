#pragma once

#include <ArduinoJson.h>

#include "balansun_config_audit_keys.h"

void balansun_config_audit_record(const char *route, JsonObjectConst body);
void balansun_config_audit_append_json(JsonArray arr, int max_entries = 20);
