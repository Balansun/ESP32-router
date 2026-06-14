#pragma once

#include <Arduino.h>

/** HTTP GET over Wi-Fi (LAN); returns body after headers or empty on failure. */
bool balansun_lan_http_get(const String &host, uint16_t port, const String &path, String &body_out,
                        uint32_t timeout_ms = 5000);
