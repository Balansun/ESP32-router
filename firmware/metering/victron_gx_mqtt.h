#pragma once

#include <ArduinoJson.h>

void victron_gx_mqtt_begin(void);
void victron_gx_mqtt_reconnect(void);
void victron_gx_mqtt_service_tick(void);
void victron_gx_mqtt_append_diagnostics(JsonObject doc, int linky_tail_max);
