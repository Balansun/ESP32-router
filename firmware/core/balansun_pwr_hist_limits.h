#pragma once

#include <cmath>

/*
 * balansun_pwr_hist_limits.h — RAM power history ring sizes (compile-time).
 * wroom32: 288×5min (24h) + 150×2s (5min). wrover/s3: 600 + 300 (48h / 10min).
 */

#ifndef BALANSUN_PWR_HIST_5MN_SLOTS
#define BALANSUN_PWR_HIST_5MN_SLOTS 600
#endif

#ifndef BALANSUN_PWR_HIST_2S_SLOTS
#define BALANSUN_PWR_HIST_2S_SLOTS 300
#endif

constexpr int kBalansunPwrHist5mnSlots = BALANSUN_PWR_HIST_5MN_SLOTS;
constexpr int kBalansunPwrHist2sSlots = BALANSUN_PWR_HIST_2S_SLOTS;

/** API window label for GET /history/power (matches ring depth). */
constexpr int kBalansunPwrHist5mnWindowHours = (BALANSUN_PWR_HIST_5MN_SLOTS * 5) / 60;

#if BALANSUN_PWR_HIST_5MN_SLOTS <= 288
inline constexpr const char kBalansunPwrHistDefaultWindow[] = "24h";
#else
inline constexpr const char kBalansunPwrHistDefaultWindow[] = "48h";
#endif

/** Quantize live probe °C to one decimal for power-history rings (0 = no sample). */
inline float balansun_history_temperature_sample(float t) {
  if (t <= -100.f) return 0.f;
  return roundf(t * 10.f) / 10.f;
}
