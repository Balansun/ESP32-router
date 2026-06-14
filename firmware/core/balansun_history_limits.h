#pragma once

/*
 * balansun_history_limits.h — Compile-time daily energy history retention (days).
 * Set per PlatformIO env: wroom32/hil=7, wrover=30, esp32s3*=90.
 */

#ifndef BALANSUN_HISTORY_DAYS_RETAINED
#define BALANSUN_HISTORY_DAYS_RETAINED 90
#endif

constexpr int kBalansunHistoryDaysRetained = BALANSUN_HISTORY_DAYS_RETAINED;
