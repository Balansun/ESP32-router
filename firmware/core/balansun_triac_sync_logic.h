#pragma once

#include "balansun_product_caps_logic.h"

#include <cstdint>

enum class BalansunTriacSyncState : uint8_t {
  NotApplicable = 0,
  Unsynced,
  Acquiring,
  Synced,
  Fallback10ms,
};

BalansunTriacSyncState balansun_triac_sync_state_from_zc(int16_t zc_sync_state, const BalansunProductCaps &caps);

const char *balansun_triac_sync_state_wire(BalansunTriacSyncState state);
