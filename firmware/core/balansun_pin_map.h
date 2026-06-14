#pragma once

#include <cstdint>

/** DS18B20 disabled (pin_temp only). Legacy EEPROM -1 on core pins is migrated at load. */
constexpr int8_t kBalansunPinTempDisabled = -1;

struct BalansunPinMap {
  int8_t triac_dim = 0;
  int8_t zero_cross = 0;
  int8_t uart_rx = 0;
  int8_t uart_tx = 0;
  int8_t temp = 0;
  int8_t analog0 = 0;
  int8_t analog1 = 0;
  int8_t analog2 = 0;
};

/** Compile-time default PCB pinout for the active IDF target. */
BalansunPinMap balansun_board_default_pins();

/** `"wroom32"` (WROOM/WROVER classic ESP32) or `"esp32s3"`. */
const char *balansun_board_pin_profile_id();

/** Replace legacy -1 on the seven core pins (not temp) with board defaults. */
void balansun_pin_map_resolve_legacy_core_pins(BalansunPinMap &map);

bool balansun_pin_map_equal(const BalansunPinMap &a, const BalansunPinMap &b);

/** Active runtime map (applied at boot). */
extern BalansunPinMap g_pins;

/** Stored assignment; persisted in EEPROM 0xE210. Core pins are concrete GPIO numbers. */
extern int8_t pinTriacDim;
extern int8_t pinZeroCross;
extern int8_t pinUartRx;
extern int8_t pinUartTx;
extern int8_t pinTempGpio;
extern int8_t pinAnalog0;
extern int8_t pinAnalog1;
extern int8_t pinAnalog2;
extern bool hwPinsPersistPresent;
extern bool pinMapRebootPending;
extern bool pinMapFallbackActive;

BalansunPinMap balansun_pin_map_from_stored();

/** Normalize globals after EEPROM load (legacy -1 sentinels). */
void balansun_pins_migrate_eeprom_load();

/** Build g_pins from stored globals; validate; fallback to defaults on failure. */
void balansun_pins_apply_boot();

void balansun_pins_early_safe_defaults();

void balansun_pins_reset_stored_to_defaults();

void balansun_pins_update_reboot_pending();
