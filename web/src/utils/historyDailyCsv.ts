import type { HistoryEnergyDaily } from "../api/types";
import { energyDayIsoDates, historyDailyDayCount } from "./historyEnergy";

/** Compact schema accepted by POST /api/v1/history/energy/daily/import */
export const HISTORY_DAILY_CSV_HEADER =
  "date_iso,ch1_import_wh,ch1_export_wh,ch2_import_wh,ch2_export_wh";

function whAt(arr: number[] | undefined, idx: number): number {
  const v = arr?.[idx];
  return typeof v === "number" && Number.isFinite(v) ? Math.round(v) : 0;
}

/** Build import-compatible daily history CSV (Wh integers, chronological). */
export function buildHistoryDailyImportCsv(
  j: HistoryEnergyDaily,
  deviceDateTime?: string,
): string {
  const n = historyDailyDayCount(j);
  const iso =
    j.day_dates_iso?.length === n
      ? j.day_dates_iso
      : energyDayIsoDates(n, j.reference_date_iso, deviceDateTime);
  const lines = [HISTORY_DAILY_CSV_HEADER];
  for (let i = 0; i < n; i++) {
    lines.push(
      [
        iso[i] ?? "",
        whAt(j.ch1_import_wh_per_day, i),
        whAt(j.ch1_export_wh_per_day, i),
        whAt(j.ch2_import_wh_per_day, i),
        whAt(j.ch2_export_wh_per_day, i),
      ].join(","),
    );
  }
  return `${lines.join("\n")}\n`;
}
