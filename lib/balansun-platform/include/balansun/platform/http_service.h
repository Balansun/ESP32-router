#pragma once

#include <WebServer.h>
#include <WiFiClient.h>

void balansun_http_tune_client(WiFiClient &client);
void balansun_http_pump_server(WebServer &server, int maxPasses);
void balansun_http_drain_client(WiFiClient &client, size_t payloadBytes);
