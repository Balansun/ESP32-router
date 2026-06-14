#pragma once

#include <ArduinoJson.h>

void balansun_append_product_capabilities_json(JsonObject caps);
void balansun_append_output_suspend_json(JsonObject parent);
void balansun_append_safety_lockout_json(JsonObject parent);
void balansun_append_operational_status_json(JsonObject status);
