#pragma once

#include <Arduino.h>

/** Periodic MQTT connect / discovery / state (Home Assistant). */
void publishMqttLoop(void);

/** Pmqtt source: connect/subscribe and drain inbound messages (call from main loop). */
void pmqtt_mqtt_service_tick(void);

/** PubSubClient callback for command topics. */
void callback(char *topic, byte *payload, unsigned int length);

/** Publish consolidated state JSON to the HA state topic. */
void SendDataToHomeAssistant(void);
