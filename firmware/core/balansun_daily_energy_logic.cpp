#include "balansun_daily_energy_logic.h"

#include "balansun_forward.h"

bool balansun_yesterday_daily_metrics(long *houseImportWh,
                                      long *houseExportWh,
                                      long *secondImportWh,
                                      long *secondExportWh) {
  if (!houseImportWh || !houseExportWh || !secondImportWh || !secondExportWh) return false;
  *houseImportWh = 0;
  *houseExportWh = 0;
  *secondImportWh = 0;
  *secondExportWh = 0;
  const int cap = eepromHistoryDaysCapacity();
  const int logicalIdx = balansun_daily_energy_yesterday_logical_index(cap);
  if (logicalIdx < 0) return true;
  long ch1Import = 0;
  long ch1Export = 0;
  long ch2Import = 0;
  long ch2Export = 0;
  if (!eepromHistoryReadDailyMetrics(logicalIdx, ch1Import, ch1Export, ch2Import, ch2Export)) {
    return true;
  }
  balansun_daily_energy_sanitize_metrics(ch1Import, ch1Export, ch2Import, ch2Export);
  *houseImportWh = ch1Import;
  *houseExportWh = ch1Export;
  *secondImportWh = ch2Import;
  *secondExportWh = ch2Export;
  return true;
}
