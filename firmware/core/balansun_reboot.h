#pragma once

#include <Arduino.h>

/** Schedule a reboot after `delayMs` (non-blocking; polled from `balansun_loop`). */
void RequestReboot(uint32_t delayMs);
void balansun_reboot_poll(void);
/** True between RequestReboot and ESP.restart (status LED overlay). */
bool balansun_reboot_pending(void);
