#include <balansun/platform/http_listener.h>

#include <WiFi.h>

void balansun_platform_http_invalidate(WebServer &server, BalansunHttpListenerState &state) {
  if (state.listening) {
    server.stop();
  }
  state.listening = false;
  state.bound_ip = IPAddress(0, 0, 0, 0);
}

void balansun_platform_http_ensure_listening(WebServer &server, BalansunHttpListenerState &state) {
  IPAddress listenIp(0, 0, 0, 0);
  const bool setup_ap = WiFi.getMode() != WIFI_STA && WiFi.softAPIP() != IPAddress(0, 0, 0, 0);
  if (setup_ap) {
    listenIp = WiFi.softAPIP();
  } else if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
    listenIp = WiFi.localIP();
  } else if (WiFi.softAPIP() != IPAddress(0, 0, 0, 0)) {
    listenIp = WiFi.softAPIP();
  }
  if (listenIp == IPAddress(0, 0, 0, 0)) {
    balansun_platform_http_invalidate(server, state);
    return;
  }
  if (!state.listening || listenIp != state.bound_ip) {
    if (state.listening) {
      server.stop();
      delay(20);
    }
    server.begin();
    state.listening = true;
    state.bound_ip = listenIp;
    Serial.print(F("HTTP server on port 80 — "));
    Serial.println(listenIp.toString());
  }
}
