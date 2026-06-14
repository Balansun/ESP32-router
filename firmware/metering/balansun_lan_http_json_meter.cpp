#include "balansun_lan_http_json_meter.h"

#include "api_util.h"
#include "balansun_globals.h"
#include "balansun_lan_http_client.h"
#include "balansun_meter_http_trim_logic.h"

void LanHttpJsonMeter::poll() {
  const String host = ip32ToDotted(ext_peer_ip);
  String body;
  if (!balansun_lan_http_get(host, httpPort(), httpPath(), body)) {
    markHttpFailure();
    return;
  }
  std::string inner;
  if (!balansun_meter_http_trim_json_object(std::string(body.c_str()), inner)) {
    return;
  }
  JsonFlatMeterReading rd;
  if (!parseBody(inner, rd)) {
    return;
  }
  storeRawBody(inner);
  applyFlatReading(rd);
  markPollSuccess();
}
