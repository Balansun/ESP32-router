#pragma once

#include <Arduino.h>
#include "balansun_fixed_str.h"

/** Cached 12-char lowercase hex uid from ESP factory MAC. */
const String &balansun_device_uid();

/** Replace empty / factory `balansun` MQTT device name with device_uid. Returns true if changed. */
bool balansun_apply_default_mqtt_device_name(String &name);

template <size_t N>
bool balansun_apply_default_mqtt_device_name(BalansunStr<N> &name) {
  String tmp = name.toString();
  if (!balansun_apply_default_mqtt_device_name(tmp)) return false;
  name.assign(tmp);
  return true;
}
