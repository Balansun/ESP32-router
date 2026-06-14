#pragma once

#include "balansun_hw_profile_logic.h"

#include <ArduinoJson.h>

void balansun_hw_profile_init();
BalansunHwTier balansun_hw_tier();
const BalansunHwCaps &balansun_hw_caps();
BalansunHwCaps balansun_hw_caps_now();
void balansun_hw_profile_append_device_json(JsonDocument &doc);
