#include "balansun_hw_profile_logic.h"

BalansunHwCaps balansun_hw_caps_for_tier(BalansunHwTier tier) {
  BalansunHwCaps c{};
  switch (tier) {
    case BalansunHwTier::Extended:
      c.config_export_json_cap = kHwCapConfigExportExtended;
      c.hist_power_json_cap = kHwCapHistPowerExtended;
      c.hist_abs_max_points = kHwCapHistMaxPtsExtended;
      c.history_daily_import_max = kHwCapDailyImportExtended;
      c.history_daily_page_max = 10;
      c.put_body_max = kHwCapPutBodyExtended;
      c.backup_part_json_cap = kHwCapBackupPartExtended;
      c.pmqtt_bindings_apply_cap = 8192;
      c.full_config_export = true;
      break;
    case BalansunHwTier::Standard:
      c.config_export_json_cap = kHwCapConfigExportStandard;
      c.hist_power_json_cap = kHwCapHistPowerStandard;
      c.hist_abs_max_points = kHwCapHistMaxPtsStandard;
      c.history_daily_import_max = kHwCapDailyImportStandard;
      c.history_daily_page_max = 10;
      c.put_body_max = kHwCapPutBodyStandard;
      c.backup_part_json_cap = kHwCapBackupPartStandard;
      c.pmqtt_bindings_apply_cap = 8192;
      c.full_config_export = true;
      break;
    default:
      c.config_export_json_cap = kHwCapConfigExportConstrained;
      c.hist_power_json_cap = kHwCapHistPowerConstrained;
      c.hist_abs_max_points = kHwCapHistMaxPtsConstrained;
      c.history_daily_import_max = kHwCapDailyImportConstrained;
      c.history_daily_page_max = 10;
      c.put_body_max = kHwCapPutBodyConstrained;
      c.backup_part_json_cap = kHwCapBackupPartConstrained;
      c.pmqtt_bindings_apply_cap = 4096;
      c.full_config_export = false;
      break;
  }
  return c;
}

BalansunHwTier balansun_hw_tier_from_resources(BalansunHwCeiling ceiling, bool psram_available, size_t psram_size_bytes,
                                         size_t /*free_heap_bytes*/) {
  if (ceiling == BalansunHwCeiling::Extended && psram_available && psram_size_bytes >= kHwPsramMinBytesForExtended) {
    return BalansunHwTier::Extended;
  }
  if (ceiling == BalansunHwCeiling::Extended) {
    return BalansunHwTier::Standard;
  }
  return BalansunHwTier::Constrained;
}

BalansunHwCaps balansun_hw_caps_demote_if_low_heap(BalansunHwCaps caps, size_t free_heap_bytes) {
  if (free_heap_bytes >= kHwMinFreeHeapForLargeJson) return caps;
  if (caps.hist_abs_max_points <= kHwCapHistMaxPtsConstrained) return caps;
  return balansun_hw_caps_for_tier(BalansunHwTier::Constrained);
}

BalansunHwCaps balansun_hw_caps_effective(BalansunHwCeiling ceiling, bool psram_available, size_t psram_size_bytes,
                                    size_t free_heap_bytes) {
  const BalansunHwTier tier =
      balansun_hw_tier_from_resources(ceiling, psram_available, psram_size_bytes, free_heap_bytes);
  return balansun_hw_caps_demote_if_low_heap(balansun_hw_caps_for_tier(tier), free_heap_bytes);
}

const char *balansun_hw_tier_name(BalansunHwTier tier) {
  switch (tier) {
    case BalansunHwTier::Extended:
      return "extended";
    case BalansunHwTier::Standard:
      return "standard";
    default:
      return "constrained";
  }
}
