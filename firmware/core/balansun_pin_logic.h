#pragma once

#include "balansun_pin_map.h"

#include <string>

enum class BalansunPinFunction {
  TriacDim,
  ZeroCross,
  UartRx,
  UartTx,
  Temp,
  Analog0,
  Analog1,
  Analog2,
};

bool balansun_pin_logic_is_allowed(BalansunPinFunction fn, int gpio, bool target_s3);

/** Validates resolved map (defaults applied). temp may be -1 (disabled). */
bool balansun_pin_map_validate(const BalansunPinMap &map, bool target_s3, std::string &err);
