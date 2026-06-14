#include <balansun/platform/http_service.h>

void balansun_http_tune_client(WiFiClient &client) {
  client.setNoDelay(true);
  client.setTimeout(30);
}

void balansun_http_pump_server(WebServer &server, int maxPasses) {
  if (maxPasses < 1) maxPasses = 1;
  for (int i = 0; i < maxPasses; i++) {
    server.handleClient();
  }
}

void balansun_http_drain_client(WiFiClient &client, size_t payloadBytes) {
  if (!client) return;
  if (payloadBytes < 1024) return;
  unsigned long drainMs = 2000UL;
  if (payloadBytes >= 2048) drainMs = 4000UL;
  if (payloadBytes >= 4096) drainMs = 6000UL;
  const unsigned long until = millis() + drainMs;
  while (client.connected() && (long)(until - millis()) > 0) {
    client.flush();
    yield();
    delay(1);
  }
}
