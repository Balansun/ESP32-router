#pragma once

#include "balansun_product_profile.h"

/**
 * Balansun — board profile for ESP32-WROOM-32 (ESP32-D0WDQ6, 4 MB flash typical).
 * GPIO mapping for the default Balansun PCB; change only if your board differs.
 *
 * **Board vs product:** `BALANSUN_BOARD_PROFILE` selects pin defaults; `BALANSUN_PRODUCT_PROFILE`
 * (in balansun_product_profile.h) selects regulation/meter capabilities. ESP32-32U builds
 * set `BALANSUN_BOARD_ESP32_32U` — same WROOM-class GPIO map as generic until a custom PCB differs.
 */

#ifndef Version
#define Version "0.1.0"
#endif
#ifndef HOSTNAME
#define HOSTNAME "BALANSUN-"
#endif
#ifndef kEepromLayoutInit
/** Bump forces EEPROM factory re-init (gen-2 layout). */
#define kEepromLayoutInit 903157203UL
#endif

#define WDT_TIMEOUT_SEC 180
#define kMaxRoutingActions 20
#define MAX_SIZE_T 80

/** Triac dimmer output and zero-cross input (reference build: RBDimmer DIM / Z-C). */
constexpr int kDefaultTriacDimGpio = 22;
constexpr int kDefaultZeroCrossGpio = 23;

// Backward-compatible aliases used by older firmware code.
#define kTriacDimGpio kDefaultTriacDimGpio
#define kZeroCrossGpio kDefaultZeroCrossGpio

#if CONFIG_IDF_TARGET_ESP32S3
/* ESP32-S3 + octal PSRAM: GPIO 26–37 are flash/PSRAM; use header-safe pins on DevKitC. */
#define kDefaultAnalogIn0 1
#define kDefaultAnalogIn1 2
#define kDefaultAnalogIn2 3
#define kDefaultRxd2 4
#define kDefaultTxd2 5
#else
/* Analog inputs (Analog probe inputs) */
#define kDefaultAnalogIn0 35
#define kDefaultAnalogIn1 32
#define kDefaultAnalogIn2 33
/* UART2 — Linky or JSY-MK-194 (JsyMk194) */
#define kDefaultRxd2 26
#define kDefaultTxd2 27
#endif

// Backward-compatible UART2 and analog pin aliases used by older firmware code.
#define RXD2 kDefaultRxd2
#define TXD2 kDefaultTxd2
#define AnalogIn0 kDefaultAnalogIn0
#define AnalogIn1 kDefaultAnalogIn1
#define AnalogIn2 kDefaultAnalogIn2

#define LedYellow 18
#define LedGreen 19
#if CONFIG_IDF_TARGET_ESP32S3
/** N16R8 / DevKitC-1 onboard WS2812 — GPIO 48 via Arduino RGB_BUILTIN (solder RGB pad if needed). */
constexpr int kS3OnboardRgbGpio = 48;
#define kDefaultPinTemp 21
#else
#define kDefaultPinTemp 13
#endif

#define pinTemp kDefaultPinTemp
