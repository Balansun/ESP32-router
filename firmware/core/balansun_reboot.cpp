#include "balansun_reboot.h"
#include "balansun_globals.h"
#include "balansun_persistence.h"
#include "storage_eeprom.h"
#include <esp_system.h>

static bool restartPending = false;
static uint32_t restartAtMillis = 0;

void RequestReboot(uint32_t delayMs) {
  restartPending = true;
  restartAtMillis = millis() + delayMs;
}

bool balansun_reboot_pending(void) { return restartPending; }

void balansun_reboot_poll(void) {
  if (!restartPending) {
    return;
  }
  if ((long)(millis() - restartAtMillis) < 0) {
    return;
  }
  restartPending = false;
  /* Persist full config before restart so params NVS cannot lag behind in-RAM globals. */
  if (balansun_eeprom_config_loaded) {
    persistConfigToEeprom();
  } else {
    persistence_flush_all();
  }
  ESP.restart();
}
