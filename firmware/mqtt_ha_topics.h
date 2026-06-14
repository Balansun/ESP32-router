/*
 * Shared MQTT topic helpers for Home Assistant discovery and state.
 */
#pragma once
#include "balansun_globals.h"
#include <Arduino.h>

inline String mqttStateTopic() {
  return String(MQTTPrefix) + "/" + MQTTdeviceName + "_state";
}
inline String mqttAvailabilityTopic() {
  return String(MQTTPrefix) + "/" + MQTTdeviceName + "/availability";
}
inline String mqttEventTopic() {
  return String(MQTTPrefix) + "/" + MQTTdeviceName + "/event";
}
inline String mqttErrorTopic() {
  return String(MQTTPrefix) + "/" + MQTTdeviceName + "/error";
}
