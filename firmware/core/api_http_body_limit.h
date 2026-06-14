#pragma once

#include <cstddef>

class WiFiClient;

/** Return true when a 413 JSON response was sent and the body must not be read. */
extern "C" bool balansun_api_reject_oversized_before_read(int httpMethod, const char *uri, size_t contentLength,
                                                     WiFiClient &client);
