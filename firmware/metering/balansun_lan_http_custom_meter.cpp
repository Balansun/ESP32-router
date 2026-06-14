#include "balansun_lan_http_custom_meter.h"

#include "balansun_lan_http_client.h"

void LanHttpCustomMeter::poll() {
  String host;
  String path;
  buildRequest(host, path);
  String body;
  if (!balansun_lan_http_get(host, httpPort(), path, body)) {
    markHttpFailure();
    return;
  }
  if (!parseAndApply(body)) {
    return;
  }
  markPollSuccess();
}
