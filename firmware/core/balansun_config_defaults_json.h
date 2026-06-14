#pragma once

#include <ArduinoJson.h>

constexpr const char *kBalansunConfigDefaultsProfile = "balansun_globals_v1";

bool balansun_config_value_is_factory_default(const char *key, JsonVariantConst value);
