#pragma once

#include <cstddef>
#include <cstdint>

/*
 * balansun_hw_profile_logic.h — Pure board tier + memory cap policy (host-testable).
 *
 * WROOM-32 → Constrained; WROVER / S3+PSRAM → Extended; S3 without PSRAM → Standard.
 * Compile-time ceiling (PlatformIO env) clamps the maximum tier at boot.
 */

enum class BalansunHwCeiling : uint8_t { Constrained = 0, Extended = 1 };
enum class BalansunHwTier : uint8_t { Constrained = 0, Standard = 1, Extended = 2 };

struct BalansunHwCaps {
  size_t config_export_json_cap;
  size_t hist_power_json_cap;
  int hist_abs_max_points;
  size_t history_daily_import_max;
  int history_daily_page_max;
  size_t put_body_max;
  size_t backup_part_json_cap;
  size_t pmqtt_bindings_apply_cap;
  bool full_config_export;
};

constexpr size_t kHwCapConfigExportConstrained = 20480;
constexpr size_t kHwCapConfigExportStandard = 32768;
constexpr size_t kHwCapConfigExportExtended = 49152;
constexpr size_t kHwCapPutBodyConstrained = kHwCapConfigExportConstrained;
constexpr size_t kHwCapPutBodyStandard = kHwCapConfigExportStandard;
constexpr size_t kHwCapPutBodyExtended = kHwCapConfigExportExtended;
constexpr size_t kHwCapBackupPartConstrained = 12288;
constexpr size_t kHwCapBackupPartStandard = 16384;
constexpr size_t kHwCapBackupPartExtended = 24576;
constexpr size_t kHwCapHistPowerConstrained = 12288;
constexpr size_t kHwCapHistPowerStandard = 20480;
constexpr size_t kHwCapHistPowerExtended = 32768;
constexpr int kHwCapHistMaxPtsConstrained = 200;
constexpr int kHwCapHistMaxPtsStandard = 400;
constexpr int kHwCapHistMaxPtsExtended = 600;
constexpr size_t kHwCapDailyImportConstrained = 16384;
constexpr size_t kHwCapDailyImportStandard = 24576;
constexpr size_t kHwCapDailyImportExtended = 32768;
constexpr size_t kHwPsramMinBytesForExtended = 2u * 1024u * 1024u;
constexpr size_t kHwMinFreeHeapForLargeJson = 40960;

BalansunHwTier balansun_hw_tier_from_resources(BalansunHwCeiling ceiling, bool psram_available, size_t psram_size_bytes,
                                         size_t free_heap_bytes);
BalansunHwCaps balansun_hw_caps_for_tier(BalansunHwTier tier);
BalansunHwCaps balansun_hw_caps_effective(BalansunHwCeiling ceiling, bool psram_available, size_t psram_size_bytes,
                                    size_t free_heap_bytes);
BalansunHwCaps balansun_hw_caps_demote_if_low_heap(BalansunHwCaps caps, size_t free_heap_bytes);
const char *balansun_hw_tier_name(BalansunHwTier tier);
