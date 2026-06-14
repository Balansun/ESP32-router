#pragma once

#include "balansun_product_caps_logic.h"

#include <cstdint>

enum class BalansunOutputSuspendReason : uint8_t {
  None = 0,
  SafetyLockout,
  Vacation,
  SourceStale,
  Commissioning,
};

struct BalansunOutputSuspendInput {
  BalansunProductCaps caps{};
  bool vacation_active = false;
  bool triac_off_when_source_stale = false;
  bool source_stale = false;
  bool commissioning_blocks_outputs = false;
  bool self_test_running = false;
  /** Boot / commissioning gate: suspend until this session's self-test completes. */
  bool self_test_blocks_outputs = false;
  bool safety_lockout_active = false;
};

struct BalansunOutputSuspendResult {
  bool active = false;
  BalansunOutputSuspendReason reason = BalansunOutputSuspendReason::None;
};

BalansunOutputSuspendResult balansun_output_suspend_eval(const BalansunOutputSuspendInput &in);

const char *balansun_output_suspend_reason_wire(BalansunOutputSuspendReason reason);
