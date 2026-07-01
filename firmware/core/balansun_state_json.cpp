#include "balansun_state_json.h"

#include "balansun_meter_pack.h"
#include "balansun_product_profile.h"
#include "balansun_source.h"
#include "balansun_device_lifecycle_logic.h"
#include "balansun_output_suspend_logic.h"
#include "balansun_product_caps.h"
#include "balansun_regulation_motion_logic.h"
#include "balansun_self_test_safety_logic.h"
#include "balansun_self_test_safety_runtime.h"
#include "balansun_state_snapshot.h"
#include "balansun_triac_sync_logic.h"
#include <balansun/load_profile.h>
#include "balansun_globals.h"

#include <cstring>

void balansun_append_product_capabilities_json(JsonObject caps) {
  const BalansunProductCaps pc = balansun_product_caps_compile_time();
  caps["product_profile"] = pc.profile_wire;
  JsonObject fw = caps["firmware_capabilities"].to<JsonObject>();
  const bool router = balansun_product_has_cap(BalansunCap::SurplusRegulation);
  fw["surplus_regulation"] = router;
  // Triac dimming is intrinsic to router builds; never advertise triac without surplus_regulation.
  fw["triac"] = router;
  fw["multi_action"] = balansun_product_has_cap(BalansunCap::MultiAction);
  fw["source_test_inject"] = balansun_product_has_cap(BalansunCap::SourceTestInject);
  fw["meter_pack"] = balansun_meter_pack_id_wire();
  const LoadProfile load_profile = balansun_load_profile_from_wire(loadProfileWire.c_str());
  fw["triac_dimming"] = router || balansun_load_profile_supports_triac(load_profile);
  JsonArray meters = fw["meters"].to<JsonArray>();
  const size_t n = balansun_source_registry_count();
  for (size_t i = 0; i < n; i++) {
    const char *w = balansun_source_wire_at(i);
    if (!w || !w[0]) continue;
#if BALANSUN_METER_PACK != BALANSUN_METER_PACK_FULL
    if (strcmp(w, "NotDef") == 0) continue;
#endif
    meters.add(w);
  }
}

void balansun_append_output_suspend_json(JsonObject parent) {
  JsonObject o = parent["output_suspend"].to<JsonObject>();
  o["active"] = g_balansun_state_snapshot.output_suspend.active;
  o["reason"] = balansun_output_suspend_reason_wire(g_balansun_state_snapshot.output_suspend.reason);
}

void balansun_append_safety_lockout_json(JsonObject parent) {
  const BalansunSelfTestSafetyResult safety = balansun_self_test_safety_eval_now();
  JsonObject o = parent["safety_lockout"].to<JsonObject>();
  o["active"] = false;
  JsonArray reasons = o["reasons"].to<JsonArray>();
  if (!balansun_self_test_safety_has_critical_failure(safety)) {
    return;
  }
  const char *reason_wires[3];
  size_t reason_count = 0;
  balansun_self_test_safety_lockout_reasons(safety, reason_wires, 3, &reason_count);
  for (size_t i = 0; i < reason_count; i++) {
    reasons.add(reason_wires[i]);
  }
}

void balansun_append_operational_status_json(JsonObject status) {
  status["device_lifecycle"] = balansun_device_lifecycle_wire(g_balansun_state_snapshot.lifecycle);
  status["regulation_motion"] = balansun_regulation_motion_wire(g_balansun_state_snapshot.regulation_motion);
  status["triac_sync_state"] = balansun_triac_sync_state_wire(g_balansun_state_snapshot.triac_sync);
  balansun_append_output_suspend_json(status);
}
