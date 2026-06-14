#pragma once

#include "balansun_self_test_safety_logic.h"

BalansunSelfTestSafetyResult balansun_self_test_safety_eval_now();

bool balansun_api_safety_lockout_active();
