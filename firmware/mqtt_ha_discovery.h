#pragma once

#include <Arduino.h>

void sendMQTTDiscoveryMsg_global(void);
extern bool mqtt_ha_discovered;

void DeviceToDiscover(String Name, String Unit, String Class, String Round);
void DeviceBinToDiscover(String Name, String title);
void DeviceAutomationTriggerToDiscover(const char *subtype, const char *title);
void DeviceVacationSwitchToDiscover();
void DeviceSwitchToDiscover(int index, String title);
void DeviceSensorNumberToDiscover(const String &Name, const String &title, int minValue, int maxValue);
void DeviceConfigNumberToDiscover(const String &Name, const String &title, const String &cmdSuffix,
                                  int minValue, int maxValue);
void DeviceNumberToDiscover(String Name, String title, int minValue, int maxValue);
void DeviceSelectSourceToDiscover();
void DeviceTextToDiscover(String Name, String title);
