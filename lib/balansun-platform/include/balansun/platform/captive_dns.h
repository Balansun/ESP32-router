#pragma once

#include <Arduino.h>
#include <IPAddress.h>

void balansun_captive_dns_start(const IPAddress &apIp);
void balansun_captive_dns_stop(void);
void balansun_captive_dns_process(void);
bool balansun_captive_dns_active(void);
