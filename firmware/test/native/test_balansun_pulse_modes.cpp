#include <gtest/gtest.h>

#include "balansun_pulse_modes.h"
TEST(BalansunPulseModes, SinusTableMonotonicBounds) {
  balansun_pulse_modes_init_tables();
  EXPECT_EQ(balansun_pulse_sinus_on(0), 0u);
  EXPECT_GE(balansun_pulse_sinus_on(50), 1u);
  EXPECT_LE(balansun_pulse_sinus_on(100), 100u);
  EXPECT_GE(balansun_pulse_sinus_total(50), 2u);
  // init_tables() picks minimal (T,N) pair per open %; 100% → T=N=20 in host build.
  EXPECT_EQ(balansun_pulse_sinus_total(100), 20u);
  EXPECT_EQ(balansun_pulse_sinus_on(100), 20u);
}
