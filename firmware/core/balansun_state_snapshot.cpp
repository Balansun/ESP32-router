#include "balansun_state_snapshot.h"

#include "balansun_api_ready.h"
#include "balansun_globals.h"
#include "balansun_product_caps.h"
#include "balansun_self_test.h"
#include "balansun_self_test_safety_runtime.h"
#include "balansun_triac_isr.h"

BalansunStateSnapshot g_balansun_state_snapshot;

static int g_prev_triac_open_percent = 0;

void balansun_state_snapshot_refresh(bool api_telemetry_ready, bool vacation_active, bool source_stale,
                                  bool triac_off_when_source_stale, bool self_test_running,
                                  int triac_open_percent, bool schedule_allows_surplus, int ceiling_percent) {
  const BalansunProductCaps caps = balansun_product_caps_compile_time();
  const bool commissioning_blocks = commissioningBlocksOutputs;
  const BalansunSelfTestSafetyResult safety = balansun_self_test_safety_eval_now();

  BalansunOutputSuspendInput suspend_in{};
  suspend_in.caps = caps;
  suspend_in.safety_lockout_active = safety.lockout_active;
  suspend_in.vacation_active = vacation_active;
  suspend_in.triac_off_when_source_stale = triac_off_when_source_stale;
  suspend_in.source_stale = source_stale;
  suspend_in.commissioning_blocks_outputs = commissioning_blocks;
  suspend_in.self_test_running = self_test_running;
  suspend_in.self_test_blocks_outputs = balansun_self_test_blocks_outputs();
  g_balansun_state_snapshot.output_suspend = balansun_output_suspend_eval(suspend_in);

  BalansunDeviceLifecycleInput life_in{};
  life_in.caps = caps;
  life_in.boot_complete = balansun_boot_complete;
  life_in.api_telemetry_ready = api_telemetry_ready;
  life_in.output_suspend_active = g_balansun_state_snapshot.output_suspend.active;
  life_in.self_test_running = self_test_running || suspend_in.self_test_blocks_outputs;
  g_balansun_state_snapshot.lifecycle = balansun_device_lifecycle_compute(life_in);

  BalansunRegulationMotionInput motion_in{};
  motion_in.caps = caps;
  motion_in.suspend_active = g_balansun_state_snapshot.output_suspend.active;
  motion_in.schedule_allows_surplus = schedule_allows_surplus;
  motion_in.prev_open_percent = g_prev_triac_open_percent;
  motion_in.new_open_percent = triac_open_percent;
  motion_in.ceiling_percent = ceiling_percent;
  g_balansun_state_snapshot.regulation_motion = balansun_regulation_motion_compute(motion_in);

  if (g_balansun_state_snapshot.output_suspend.active) {
    g_prev_triac_open_percent = 0;
  } else {
    g_prev_triac_open_percent = triac_open_percent;
  }

  g_balansun_state_snapshot.triac_sync =
      balansun_triac_sync_state_from_zc(zc_sync_state, caps);
}
