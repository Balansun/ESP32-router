#include <gtest/gtest.h>

#include "balansun_pwm_logic.h"

TEST(BalansunPwmLogic, ValidatesAllowedGpio) {
  std::string err;
  EXPECT_TRUE(balansun_pwm_logic_validate_gpio(-1, err));
  EXPECT_TRUE(balansun_pwm_logic_validate_gpio(4, err));
  EXPECT_FALSE(balansun_pwm_logic_validate_gpio(22, err));
}

TEST(BalansunPwmLogic, FollowTriacDuty) {
  PwmConfig cfg;
  cfg.gpio = 4;
  cfg.mode = PwmMode::FollowTriac;
  EXPECT_EQ(balansun_pwm_logic_effective_duty(cfg, 35), 35);
  cfg.inverted = true;
  EXPECT_EQ(balansun_pwm_logic_effective_duty(cfg, 35), 65);
}

TEST(BalansunPwmLogic, LedcDutyRange) {
  EXPECT_EQ(balansun_pwm_logic_ledc_duty_10bit(0), 0);
  EXPECT_EQ(balansun_pwm_logic_ledc_duty_10bit(100), 1023);
  EXPECT_EQ(balansun_pwm_logic_ledc_duty_10bit(-5), 0);
  EXPECT_EQ(balansun_pwm_logic_ledc_duty_10bit(150), 1023);
}

TEST(BalansunPwmLogic, ParseModeAndGpioEdges) {
  PwmMode mode;
  std::string err;
  EXPECT_FALSE(balansun_pwm_logic_parse_mode(nullptr, mode, err));
  EXPECT_TRUE(balansun_pwm_logic_parse_mode("follow_triac", mode, err));
  EXPECT_EQ(mode, PwmMode::FollowTriac);
  EXPECT_TRUE(balansun_pwm_logic_parse_mode("off", mode, err));
  EXPECT_EQ(mode, PwmMode::Off);
  EXPECT_TRUE(balansun_pwm_logic_parse_mode("independent", mode, err));
  EXPECT_EQ(mode, PwmMode::Independent);
  EXPECT_FALSE(balansun_pwm_logic_parse_mode("invalid", mode, err));
  EXPECT_FALSE(balansun_pwm_logic_is_allowed_gpio(22));
}

TEST(BalansunPwmLogic, EffectiveDutyModes) {
  PwmConfig cfg;
  cfg.gpio = 4;
  cfg.mode = PwmMode::Off;
  EXPECT_EQ(balansun_pwm_logic_effective_duty(cfg, 50), 0);
  cfg.gpio = -1;
  EXPECT_EQ(balansun_pwm_logic_effective_duty(cfg, 50), 0);
  cfg.gpio = 4;
  cfg.mode = PwmMode::Independent;
  cfg.duty_percent = 120;
  EXPECT_EQ(balansun_pwm_logic_effective_duty(cfg, 0), 100);
  cfg.duty_percent = -10;
  EXPECT_EQ(balansun_pwm_logic_effective_duty(cfg, 0), 0);
}

TEST(BalansunPwmLogic, AllowedGpioList) {
  EXPECT_TRUE(balansun_pwm_logic_is_allowed_gpio(5));
  EXPECT_TRUE(balansun_pwm_logic_is_allowed_gpio(14));
  EXPECT_TRUE(balansun_pwm_logic_is_allowed_gpio(25));
  EXPECT_TRUE(balansun_pwm_logic_is_allowed_gpio(16));
  EXPECT_FALSE(balansun_pwm_logic_is_allowed_gpio(-1));
  std::string err;
  EXPECT_TRUE(balansun_pwm_logic_validate_gpio(17, err));
  EXPECT_TRUE(balansun_pwm_logic_validate_gpio(21, err));
}
