#include "balansun_meter_http_trim_logic.h"

bool balansun_meter_http_trim_json_object(const std::string &raw, std::string &inner_out) {
  inner_out.clear();
  const size_t start = raw.find('{');
  if (start == std::string::npos) return false;
  const size_t end = raw.find('}', start + 1);
  if (end == std::string::npos || end <= start + 1) return false;
  inner_out = raw.substr(start + 1, end - start - 1);
  return true;
}
