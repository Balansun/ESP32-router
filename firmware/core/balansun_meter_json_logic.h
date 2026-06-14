#pragma once

#include <string>

/** Host-side validation for inject JSON (shape like /api/v1/measurements). */
bool balansun_meter_json_logic_validate_inject(const std::string &json, std::string *err_out);
