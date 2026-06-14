#pragma once

#include <ArduinoJson.h>

/** Periodic sampling (ZC edge rate); call from balansun_loop. */
void balansun_hw_presence_tick(unsigned long now_ms);

/** JSY Modbus poll outcome (metering task / core 0). */
void balansun_hw_presence_on_jsy_poll(bool uart_ready, bool frame_ok);

/** DS18B20 read outcome. */
void balansun_hw_presence_on_temp_poll(bool ok);

/** Append `hardware.items[]` to GET /api/v1/health and diagnostics consumers. */
void balansun_hw_presence_append_json(JsonObject root);

/** Force a fresh hardware check for one item; writes a single item object into out_item. */
bool balansun_hw_presence_recheck(const char *id, JsonObject out_item);
