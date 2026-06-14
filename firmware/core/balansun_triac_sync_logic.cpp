#include "balansun_triac_sync_logic.h"

BalansunTriacSyncState balansun_triac_sync_state_from_zc(int16_t zc_sync_state, const BalansunProductCaps &caps) {
  if (!balansun_product_caps_has(caps, BalansunCap::TriacDimming)) {
    return BalansunTriacSyncState::NotApplicable;
  }
  if (zc_sync_state < 0) {
    return BalansunTriacSyncState::Fallback10ms;
  }
  if (zc_sync_state >= 5) {
    return BalansunTriacSyncState::Synced;
  }
  if (zc_sync_state > 0) {
    return BalansunTriacSyncState::Acquiring;
  }
  return BalansunTriacSyncState::Unsynced;
}

const char *balansun_triac_sync_state_wire(BalansunTriacSyncState state) {
  switch (state) {
    case BalansunTriacSyncState::Unsynced:
      return "unsynced";
    case BalansunTriacSyncState::Acquiring:
      return "acquiring";
    case BalansunTriacSyncState::Synced:
      return "synced";
    case BalansunTriacSyncState::Fallback10ms:
      return "fallback_10ms";
    case BalansunTriacSyncState::NotApplicable:
    default:
      return "not_applicable";
  }
}
