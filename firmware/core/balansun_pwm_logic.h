#pragma once

/*
 * balansun_pwm_logic.h — Optional dedicated PWM channel (GPIO, mode, duty); host-testable.
 * User: dedicated PWM output — /fr/user-guide/#guide-b3-sortie-pwm-matérielle-dédiée
 */

#include <cstdint>
#include <string>

/** 0 off, 1 follow_triac, 2 independent */
enum class PwmMode : uint8_t { Off = 0, FollowTriac = 1, Independent = 2 };

struct PwmConfig {
  int gpio = -1;
  PwmMode mode = PwmMode::Off;
  int duty_percent = 0;
  bool inverted = false;
};

bool balansun_pwm_logic_is_allowed_gpio(int gpio);
bool balansun_pwm_logic_validate_gpio(int gpio, std::string &err);
bool balansun_pwm_logic_parse_mode(const char *s, PwmMode &out, std::string &err);
int balansun_pwm_logic_effective_duty(const PwmConfig &cfg, int triac_open_percent);
int balansun_pwm_logic_ledc_duty_10bit(int percent);
