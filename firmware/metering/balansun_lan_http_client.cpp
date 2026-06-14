#include "balansun_meter_transport_guards.h"

#if BALANSUN_METER_NEEDS_LAN_HTTP

#include "balansun_lan_http_client.h"

#include "balansun_globals.h"
#include "balansun_http_response_logic.h"
#include "balansun_http_service.h"

#include <WiFi.h>

bool balansun_lan_http_get(const String &host, uint16_t port, const String &path, String &body_out,
                        uint32_t timeout_ms) {
  body_out = "";
  WiFiClient client;
  if (!client.connect(host.c_str(), port)) {
    return false;
  }
  balansun_http_tune_client(client);
  // HTTP/1.0 + Connection: close avoids chunked bodies on many public servers (parse bug otherwise).
  client.print(String("GET ") + path + " HTTP/1.0\r\nHost: " + host +
                 "\r\nConnection: close\r\nUser-Agent: Balansun/1.0\r\nAccept: */*\r\n\r\n");

  const unsigned long start = millis();
  String wire;
  wire.reserve(768);
  while (client.connected() || client.available()) {
    if (client.available()) {
      while (client.available() && wire.length() < 8192) {
        wire += static_cast<char>(client.read());
      }
    }
    if (!client.connected() && !client.available()) {
      break;
    }
    if (millis() - start > timeout_ms) {
      client.stop();
      return false;
    }
    balansun_http_pump_server(server, 4);
    yield();
  }
  client.stop();

  const std::string body = balansun_http_response_extract_body(std::string(wire.c_str()));
  if (body.empty()) {
    return false;
  }
  body_out = String(body.c_str());
  return true;
}

#endif
