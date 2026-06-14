#include "balansun_serial_meter_driver.h"

#include <esp_task_wdt.h>

void SerialMeterDriver::poll() {
  if (!pollTransport()) {
    return;
  }
  esp_task_wdt_reset();
}
