/* balansun_pwm_logic.cpp — PWM GPIO allow-list and duty mapping (native tests). */
#include "balansun_pwm_logic.h"

#include <cstring>

bool balansun_pwm_logic_is_allowed_gpio(int gpio) {
  if (gpio < 0) return false;
  if (gpio == 4 || gpio == 5 || gpio == 14 || gpio == 16 || gpio == 17 || gpio == 21 || gpio == 25)
    return true;
  return false;
}

bool balansun_pwm_logic_validate_gpio(int gpio, std::string &err) {
  if (gpio < 0) return true;
  if (!balansun_pwm_logic_is_allowed_gpio(gpio)) {
    err = "pwm_gpio not allowed (use 4,5,14,16,17,21,25 or -1 to disable)";
    return false;
  }
  return true;
}

bool balansun_pwm_logic_parse_mode(const char *s, PwmMode &out, std::string &err) {
  if (!s) {
    err = "pwm_mode required";
    return false;
  }
  if (strcmp(s, "off") == 0) {
    out = PwmMode::Off;
    return true;
  }
  if (strcmp(s, "follow_triac") == 0) {
    out = PwmMode::FollowTriac;
    return true;
  }
  if (strcmp(s, "independent") == 0) {
    out = PwmMode::Independent;
    return true;
  }
  err = "pwm_mode must be off, follow_triac, or independent";
  return false;
}

int balansun_pwm_logic_effective_duty(const PwmConfig &cfg, int triac_open_percent) {
  if (cfg.mode == PwmMode::Off || cfg.gpio < 0) return 0;
  int duty = cfg.mode == PwmMode::FollowTriac ? triac_open_percent : cfg.duty_percent;
  if (duty < 0) duty = 0;
  if (duty > 100) duty = 100;
  if (cfg.inverted) duty = 100 - duty;
  return duty;
}

int balansun_pwm_logic_ledc_duty_10bit(int percent) {
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  return (percent * 1023) / 100;
}
