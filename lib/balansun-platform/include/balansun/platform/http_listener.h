#pragma once

#include <Arduino.h>
#include <WebServer.h>

struct BalansunHttpListenerState {
  bool listening = false;
  IPAddress bound_ip;
};

/** Bind :80 on soft-AP or STA IP; rebind when the interface changes. */
void balansun_platform_http_ensure_listening(WebServer &server, BalansunHttpListenerState &state);

void balansun_platform_http_invalidate(WebServer &server, BalansunHttpListenerState &state);
