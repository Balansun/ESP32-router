#pragma once

#include "balansun_channel_state_logic.h"
#include "balansun_device_lifecycle_logic.h"
#include "balansun_output_suspend_logic.h"
#include "balansun_regulation_motion_logic.h"
#include "balansun_triac_sync_logic.h"

/** Last computed operational state (core 1 regulation tick). */
struct BalansunStateSnapshot {
  BalansunDeviceLifecycle lifecycle = BalansunDeviceLifecycle::Booting;
  BalansunOutputSuspendResult output_suspend{};
  BalansunRegulationMotion regulation_motion = BalansunRegulationMotion::Idle;
  BalansunTriacSyncState triac_sync = BalansunTriacSyncState::NotApplicable;
};

extern BalansunStateSnapshot g_balansun_state_snapshot;

void balansun_state_snapshot_refresh(bool api_telemetry_ready, bool vacation_active, bool source_stale,
                                  bool triac_off_when_source_stale, bool self_test_running,
                                  int triac_open_percent, bool schedule_allows_surplus, int ceiling_percent);
