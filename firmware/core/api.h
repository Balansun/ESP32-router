#pragma once
#include <Arduino.h>

void Init_ApiRoutes();
/** Handle GET/PUT/PATCH /api/v1/actions/config/{index} before generic 404. */
bool Api_handle_actions_config_subresource();
bool Api_handle_auth_tokens_subresource();
bool Api_handle_backup_subresource();
bool ApiSetActionOverride(int idx, const char *state, int triacPercent, unsigned long durationSec, String &err);
bool ApiSetSource(const String &nextSource, bool persist, String &err);
void ApiMqttRepublishDiscovery();
void ApiMqttReconnect();
void ApiMqttPublishNow();
/** Synchronous broker connect test (optional overrides; does not persist config). */
bool ApiMqttTestConnection(const String &host, uint16_t port, const String &user, const String &pwd,
                           const String &deviceName, int &errorCodeOut, String &messageOut);
