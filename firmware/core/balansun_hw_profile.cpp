#include "balansun_hw_profile.h"

#include "balansun_state_json.h"
#include "balansun_forward.h"
#include "balansun_history_limits.h"
#include "balansun_pwr_hist_limits.h"
#include "balansun_psram.h"

#include <Arduino.h>
#include <ArduinoJson.h>

static BalansunHwCaps g_caps{};
static BalansunHwTier g_tier = BalansunHwTier::Constrained;

static BalansunHwCeiling compile_ceiling() {
#if defined(BALANSUN_HW_CEILING_EXTENDED)
  return BalansunHwCeiling::Extended;
#else
  return BalansunHwCeiling::Constrained;
#endif
}

void balansun_hw_profile_init() {
  const BalansunHwCeiling ceiling = compile_ceiling();
  const bool psram = balansun_psram_available();
  const size_t psram_size = balansun_psram_size();
  const size_t free_heap = ESP.getFreeHeap();
  g_tier = balansun_hw_tier_from_resources(ceiling, psram, psram_size, free_heap);
  g_caps = balansun_hw_caps_effective(ceiling, psram, psram_size, free_heap);
  Serial.printf("[hw] tier=%s psram=%u free_heap=%u config_cap=%u hist_pts=%d\n", balansun_hw_tier_name(g_tier),
                static_cast<unsigned>(psram_size), static_cast<unsigned>(free_heap),
                static_cast<unsigned>(g_caps.config_export_json_cap), g_caps.hist_abs_max_points);
}

BalansunHwTier balansun_hw_tier() { return g_tier; }

const BalansunHwCaps &balansun_hw_caps() { return g_caps; }

BalansunHwCaps balansun_hw_caps_now() { return balansun_hw_caps_demote_if_low_heap(g_caps, ESP.getFreeHeap()); }

void balansun_hw_profile_append_device_json(JsonDocument &doc) {
  const BalansunHwCaps &c = balansun_hw_caps();
  JsonObject cap = doc["capabilities"].to<JsonObject>();
  cap["tier"] = balansun_hw_tier_name(g_tier);
  cap["psram"] = balansun_psram_available();
  cap["hist_max_points"] = c.hist_abs_max_points;
  cap["history_days_retained"] = kBalansunHistoryDaysRetained;
  cap["history_days_capacity"] = eepromHistoryDaysCapacity();
  cap["history_power_window"] = kBalansunPwrHistDefaultWindow;
  cap["config_export_max_bytes"] = c.config_export_json_cap;
  cap["put_body_max_bytes"] = c.put_body_max;
  cap["full_config_export"] = c.full_config_export;
  cap["backup_part_max_bytes"] = c.backup_part_json_cap;
  if (c.full_config_export) {
    cap["backup_export_mode"] = "monolithic";
  } else {
    cap["backup_export_mode"] = "parts";
    cap["backup_part_count"] = 3;
  }
  balansun_append_product_capabilities_json(cap);
}
