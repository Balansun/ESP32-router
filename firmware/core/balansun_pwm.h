#pragma once

/* balansun_pwm.h — LEDC PWM output; loads config from balansun_globals after EEPROM. */

#include "balansun_pwm_logic.h"

void balansun_pwm_load_from_globals();
void balansun_pwm_hw_reinit();
void balansun_pwm_tick(int triac_open_percent);
