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

/** Reads persisted daily ring slot for yesterday; zeros when cap < 2. */
bool balansun_yesterday_daily_metrics(long *houseImportWh,
                                      long *houseExportWh,
                                      long *secondImportWh,
                                      long *secondExportWh);
