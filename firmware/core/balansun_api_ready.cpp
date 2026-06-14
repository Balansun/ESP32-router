#include "balansun_api_ready.h"

#include "api_util.h"
#include "balansun_api_ready_logic.h"
#include "balansun_device_lifecycle_logic.h"
#include "balansun_globals.h"
#include "balansun_product_caps.h"
#include "balansun_source.h"
#include "balansun_source_types.h"
#include "balansun_self_test.h"
#include "balansun_state_snapshot.h"
#include "metering/pmqtt_bindings.h"

bool balansun_boot_complete = false;

void balansun_boot_mark_started(void) { balansun_boot_complete = false; }

void balansun_boot_mark_complete(void) { balansun_boot_complete = true; }

static bool balansun_source_meter_payload_ready(void) {
  const SourceId id = balansun_active_source_get();
  if (id == SourceId::Pmqtt) {
    if (pmqtt_bindings_config_has_enabled_entries()) {
      return pmqtt_bindings_meter_ready();
    }
    return LastPwMQTTMillis > 0;
  }
  if (id == SourceId::BalansunPeer) {
    return ext_peer_last_poll_ok;
  }
  return true;
}

bool balansun_api_telemetry_ready(void) {
  if (!balansun_boot_complete || last_metering_task_ms == 0 || !meter_reading_valid) {
    return false;
  }
  return balansun_source_meter_payload_ready();
}

const char *balansun_api_telemetry_not_ready_lifecycle_wire(void) {
  BalansunDeviceLifecycleInput in{};
  in.caps = balansun_product_caps_compile_time();
  in.boot_complete = balansun_boot_complete;
  in.api_telemetry_ready = balansun_api_ready_logic_base(balansun_boot_complete, last_metering_task_ms,
                                                      meter_reading_valid) &&
                           balansun_source_meter_payload_ready();
  in.output_suspend_active = g_balansun_state_snapshot.output_suspend.active;
  in.self_test_running = balansun_self_test_is_running();
  return balansun_device_lifecycle_wire(balansun_device_lifecycle_compute(in));
}
