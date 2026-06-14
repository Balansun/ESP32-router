#pragma once

#include <Arduino.h>
#include <WiFiClient.h>
#include <cstddef>

/** WebServer patch hooks — store malloc'd POST body per request. */
extern "C" void balansun_api_begin_request(void);
extern "C" void balansun_api_store_plain_body(char *data, size_t len);

void api_http_clear_plain_body(void);
size_t api_http_plain_body_length(void);
const char *api_http_plain_body_data(void);
bool api_http_take_plain_body(String &out);
