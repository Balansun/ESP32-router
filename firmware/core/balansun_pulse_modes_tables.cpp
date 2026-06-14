#include "balansun_pulse_modes.h"

#include <cmath>
#include <cstdint>

namespace {

uint8_t g_tab_pulse_sinus_total[101];
uint8_t g_tab_pulse_sinus_on[101];

static const uint8_t kDefaultPulseSinusTotal[101] = {
    2,  61, 43, 33, 25, 40, 33, 57, 37, 11, 20, 55, 25, 23, 57, 40, 25, 53, 61, 21, 5,  19, 59, 61, 25, 8,  23,
    37, 25, 31, 20, 29, 47, 61, 59, 40, 25, 27, 29, 59, 5,  61, 19, 51, 59, 40, 37, 17, 25, 51, 4,  51, 25, 17,
    37, 40, 59, 51, 19, 61, 5,  59, 29, 27, 25, 40, 59, 61, 47, 29, 20, 31, 25, 37, 23, 8,  25, 61, 59, 19, 5,
    21, 61, 53, 25, 40, 57, 23, 25, 55, 20, 11, 37, 57, 33, 40, 25, 33, 43, 61, 2};
static const uint8_t kDefaultPulseSinusOn[101] = {
    0,  1,  1,  1,  1,  2,  2,  4,  3,  1,  2,  6,  3,  3,  8,  6,  4,  9,  11, 4,  1,  4,  13, 14, 6,  2,  6,
    10, 7,  9,  6,  9,  15, 20, 20, 14, 9,  10, 11, 23, 2,  25, 8,  22, 26, 18, 17, 8,  12, 25, 2,  26, 13, 9,
    20, 22, 33, 29, 11, 36, 3,  36, 18, 17, 16, 26, 39, 41, 32, 20, 14, 22, 18, 27, 17, 6,  19, 47, 46, 15, 4,
    17, 50, 44, 21, 34, 49, 20, 22, 49, 18, 10, 34, 53, 31, 38, 24, 32, 42, 60, 2};

}  // namespace

void balansun_pulse_modes_init_tables(void) {
  for (int i = 0; i < 101; i++) {
    g_tab_pulse_sinus_total[i] = kDefaultPulseSinusTotal[i];
    g_tab_pulse_sinus_on[i] = kDefaultPulseSinusOn[i];
  }
  for (int I = 0; I < 101; I++) {
    const float target = static_cast<float>(I) / 100.0f;
    g_tab_pulse_sinus_total[I] = 255;
    g_tab_pulse_sinus_on[I] = 255;
    for (int T = 20; T < 101; T++) {
      for (int N = 0; N <= T; N++) {
        if (T % 2 == 1 || N % 2 == 0) {
          const float vrai = static_cast<float>(N) / static_cast<float>(T);
          const float erreur = std::fabs(vrai - target);
          if (erreur < 0.004f) {
            g_tab_pulse_sinus_total[I] = static_cast<uint8_t>(T);
            g_tab_pulse_sinus_on[I] = static_cast<uint8_t>(N);
            goto next_i;
          }
        }
      }
    }
  next_i:;
    if (g_tab_pulse_sinus_total[I] == 255) {
      g_tab_pulse_sinus_total[I] = kDefaultPulseSinusTotal[I];
      g_tab_pulse_sinus_on[I] = kDefaultPulseSinusOn[I];
    }
  }
}

uint8_t balansun_pulse_sinus_on(uint8_t open_percent) {
  if (open_percent > 100) open_percent = 100;
  return g_tab_pulse_sinus_on[open_percent];
}

uint8_t balansun_pulse_sinus_total(uint8_t open_percent) {
  if (open_percent > 100) open_percent = 100;
  return g_tab_pulse_sinus_total[open_percent];
}
