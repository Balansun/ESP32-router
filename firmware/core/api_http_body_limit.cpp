#include "api_http_body_limit.h"

#include "balansun_hw_profile.h"

#include <WebServer.h>
#include <WiFiClient.h>

#include <cstring>

namespace {

constexpr size_t kDefaultJsonBodyMax = 16384;
constexpr size_t kHistoryDailyImportBodyMax = 32768;
constexpr size_t kInjectBodyMax = 2048;
constexpr size_t kPmqttPreviewBodyMax = 4096;
constexpr size_t kMqttTestBodyMax = 384;
constexpr size_t kSmallBodyMax = 512;
constexpr size_t kTinyBodyMax = 256;
constexpr size_t kAuthLoginBodyMax = 128;

bool uri_starts_with(const char *uri, const char *prefix) { return uri && prefix && strncmp(uri, prefix, strlen(prefix)) == 0; }

size_t route_body_limit(int httpMethod, const char *uri) {
  if (!uri) return kDefaultJsonBodyMax;
  if (httpMethod == HTTP_GET || httpMethod == HTTP_HEAD || httpMethod == HTTP_OPTIONS) {
    return static_cast<size_t>(-1);
  }

  const BalansunHwCaps caps = balansun_hw_caps_now();

  if (uri_starts_with(uri, "/api/v1/history/energy/daily/import")) return caps.history_daily_import_max;
  if (uri_starts_with(uri, "/api/v1/sources/test/inject")) return kInjectBodyMax;
  if (uri_starts_with(uri, "/api/v1/sources/pmqtt/preview")) return kPmqttPreviewBodyMax;
  if (uri_starts_with(uri, "/api/v1/mqtt/test")) return kMqttTestBodyMax;
  if (uri_starts_with(uri, "/api/v1/hardware/status-led/test")) return kSmallBodyMax;
  if (uri_starts_with(uri, "/api/v1/auth/login")) return kAuthLoginBodyMax;
  if (uri_starts_with(uri, "/api/v1/auth/tokens")) return kTinyBodyMax;
  if (uri_starts_with(uri, "/api/v1/wifi")) return kSmallBodyMax;
  if (uri_starts_with(uri, "/api/v1/time")) return kSmallBodyMax;
  if (uri_starts_with(uri, "/api/v1/system/arduino-ota")) return kSmallBodyMax;
  if (httpMethod == HTTP_PUT && uri && strcmp(uri, "/api/v1/system/pins") == 0) return kSmallBodyMax;
  if (uri_starts_with(uri, "/api/v1/pwm")) return kSmallBodyMax;
  if (uri_starts_with(uri, "/api/v1/system/http-auth")) return kAuthLoginBodyMax;
  if (uri_starts_with(uri, "/api/v1/triac/override")) return kTinyBodyMax;
  if (uri_starts_with(uri, "/api/v1/gpio")) return kTinyBodyMax;
  if (uri_starts_with(uri, "/api/v1/fleet/trust-key")) return kTinyBodyMax;
  if (uri_starts_with(uri, "/api/v1/actions/") && strstr(uri, "/override")) return kTinyBodyMax;
  if (uri_starts_with(uri, "/api/v1/config")) return caps.put_body_max;
  if (uri_starts_with(uri, "/api/v1/actions/config")) return caps.put_body_max;
  if (uri_starts_with(uri, "/api/v1/system/backup/part/")) return caps.backup_part_json_cap;
  if (uri_starts_with(uri, "/api/v1/system/backup")) return caps.put_body_max;
  if (uri_starts_with(uri, "/api/v1/fleet/import")) return kDefaultJsonBodyMax;
  if (uri_starts_with(uri, "/api/v1/firmware/ota")) return static_cast<size_t>(-1);

  return kDefaultJsonBodyMax;
}

void send_payload_too_large(WiFiClient &client) {
  static const char kBody[] = "{\"error\":\"payload_too_large\",\"message\":\"body exceeds limit\"}";
  const size_t bodyLen = strlen(kBody);
  client.print(F("HTTP/1.1 413 Payload Too Large\r\n"
                   "Content-Type: application/json\r\n"
                   "Connection: close\r\n"
                   "Cache-Control: no-store\r\n"
                   "Content-Length: "));
  client.print(bodyLen);
  client.print(F("\r\n\r\n"));
  client.print(kBody);
  client.flush();
  client.stop();
}

}  // namespace

extern "C" bool balansun_api_reject_oversized_before_read(int httpMethod, const char *uri, size_t contentLength,
                                                     WiFiClient &client) {
  if (contentLength == 0) return false;
  const size_t maxLen = route_body_limit(httpMethod, uri);
  if (contentLength <= maxLen) return false;
  send_payload_too_large(client);
  return true;
}
