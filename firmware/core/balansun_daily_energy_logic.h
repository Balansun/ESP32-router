#pragma once

/** logicalIdx for yesterday in eepromHistoryReadDailyMetrics (daysAgo=1); -1 when unavailable. */
inline int balansun_daily_energy_yesterday_logical_index(int capacity) {
  if (capacity < 2) return -1;
  return capacity - 2;
}

inline void balansun_daily_energy_sanitize_metrics(long &ch1ImportWh,
                                                   long &ch1ExportWh,
                                                   long &ch2ImportWh,
                                                   long &ch2ExportWh) {
  if (ch1ImportWh < 0) ch1ImportWh = 0;
  if (ch1ExportWh < 0) ch1ExportWh = 0;
  if (ch2ImportWh < 0) ch2ImportWh = 0;
  if (ch2ExportWh < 0) ch2ExportWh = 0;
}

/** True meter rollover: large backward jump only (not OTA glitch / stale read). */
inline bool balansun_daily_energy_is_rollover(long cumulative_wh, long j0_anchor_wh) {
  if (j0_anchor_wh <= 0 || cumulative_wh <= 0 || cumulative_wh >= j0_anchor_wh) {
    return false;
  }
  const long drop = j0_anchor_wh - cumulative_wh;
  return cumulative_wh < (j0_anchor_wh / 2) || drop > 10000L;
}

void balansun_daily_energy_reset_day_floors(void);

inline long balansun_daily_energy_wh(long cumulative_wh, long &j0_anchor_wh, long &day_floor_wh) {
  if (cumulative_wh <= 0) {
    // ponytail: ignore pre-poll zeros — must not reset a loaded EEPROM anchor to 0.
    return day_floor_wh;
  }
  if (j0_anchor_wh == 0) {
    j0_anchor_wh = cumulative_wh;
    day_floor_wh = 0;
    return 0;
  }
  if (cumulative_wh < j0_anchor_wh) {
    if (balansun_daily_energy_is_rollover(cumulative_wh, j0_anchor_wh)) {
      j0_anchor_wh = cumulative_wh;
      day_floor_wh = 0;
      return 0;
    }
    // ponytail: post-OTA dip or Modbus glitch — hold J0 and published day floor.
    return day_floor_wh;
  }
  const long day = cumulative_wh - j0_anchor_wh;
  if (day > day_floor_wh) {
    day_floor_wh = day;
  }
  return day_floor_wh;
}

/** Routed CH2 day Wh — max of import/export day counters (CT orientation). */
inline double balansun_routed_day_energy_wh(double day_import_wh, double day_export_wh) {
  return day_import_wh > day_export_wh ? day_import_wh : day_export_wh;
}

/** Reads persisted daily ring slot for yesterday; zeros when cap < 2. */
bool balansun_yesterday_daily_metrics(long *houseImportWh,
                                      long *houseExportWh,
                                      long *secondImportWh,
                                      long *secondExportWh);
