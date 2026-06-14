#pragma once

#include <string>

/** Extract inner JSON object body between first `{` and matching `}` (LAN HTTP meters). */
bool balansun_meter_http_trim_json_object(const std::string &raw, std::string &inner_out);
