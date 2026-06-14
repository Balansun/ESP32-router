/*
 * balansun_pwm.cpp — Apply PwmConfig to LEDC on pwmGpio (follow_triac or independent duty).
 */
#include "balansun_pwm.h"

#include "balansun_globals.h"
#include "triac_api_shim.h"

#include <Arduino.h>

static PwmConfig g_pwm;
static int g_ledc_channel = 0;
static bool g_ledc_ready = false;

void balansun_pwm_load_from_globals() {
  g_pwm.gpio = pwmGpio;
  if (pwmMode == "follow_triac") {
    g_pwm.mode = PwmMode::FollowTriac;
  } else if (pwmMode == "independent") {
    g_pwm.mode = PwmMode::Independent;
  } else {
    g_pwm.mode = PwmMode::Off;
  }
  g_pwm.duty_percent = pwmDutyPercent;
  g_pwm.inverted = pwmInverted;
}

void balansun_pwm_hw_reinit() {
  g_ledc_ready = false;
  if (g_pwm.gpio < 0 || g_pwm.mode == PwmMode::Off) return;
  ledcSetup(g_ledc_channel, 4000, 10);
  ledcAttachPin((uint8_t)g_pwm.gpio, g_ledc_channel);
  g_ledc_ready = true;
}

void balansun_pwm_tick(int triac_open_percent) {
  balansun_pwm_load_from_globals();
  if (g_pwm.gpio < 0 || g_pwm.mode == PwmMode::Off) {
    if (g_ledc_ready) {
      ledcWrite(g_ledc_channel, 0);
    }
    return;
  }
  if (!g_ledc_ready) balansun_pwm_hw_reinit();
  if (!g_ledc_ready) return;
  const int duty = balansun_pwm_logic_effective_duty(g_pwm, triac_open_percent);
  ledcWrite(g_ledc_channel, balansun_pwm_logic_ledc_duty_10bit(duty));
}
