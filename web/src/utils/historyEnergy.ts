/** Map firmware daily energy ring buffer indices to ISO calendar dates. */

import type { HistoryEnergyDaily } from "../api/types";

/** UI/API span for daily energy table and chart (device policy). */
export function historyEnergyDaySpan(
  energy: HistoryEnergyDaily | null,
  capabilitiesRetained?: number,
): number {
  const fromEnergy = energy?.days_retained ?? energy?.days_capacity;
  if (typeof fromEnergy === "number" && fromEnergy > 0) return fromEnergy;
  if (typeof capabilitiesRetained === "number" && capabilitiesRetained > 0) {
    return capabilitiesRetained;
  }
  return 90;
}

/** Day slot count from API arrays (prefer explicit dates, then CH metrics). */
export function historyDailyDayCount(j: HistoryEnergyDaily): number {
  const dated = j.day_dates_iso?.length ?? 0;
  if (dated > 0) return dated;
  const ch1 = j.ch1_import_wh_per_day?.length ?? 0;
  const ch2 = j.ch2_import_wh_per_day?.length ?? 0;
  return Math.max(ch1, ch2, j.delta_wh_per_day?.length ?? 0);
}

/** Device clock: `dd/mm/yyyy HH:MM:SS` (see firmware sync_clock_str). */
export function parseDeviceDateTime(s: string): Date | null {
  const m = /^(\d{1,2})\/(\d{1,2})\/(\d{4})(?:\s+(\d{1,2}):(\d{2})(?::(\d{2}))?)?$/.exec(
    s.trim(),
  );
  if (!m) return null;
  const day = Number(m[1]);
  const month = Number(m[2]) - 1;
  const year = Number(m[3]);
  const hour = m[4] != null ? Number(m[4]) : 0;
  const min = m[5] != null ? Number(m[5]) : 0;
  const sec = m[6] != null ? Number(m[6]) : 0;
  const d = new Date(year, month, day, hour, min, sec);
  if (
    d.getFullYear() !== year ||
    d.getMonth() !== month ||
    d.getDate() !== day
  ) {
    return null;
  }
  return d;
}

export function toIsoDateLocal(d: Date): string {
  const y = d.getFullYear();
  const mo = String(d.getMonth() + 1).padStart(2, "0");
  const da = String(d.getDate()).padStart(2, "0");
  return `${y}-${mo}-${da}`;
}

/**
 * `delta_wh_per_day` from firmware: index 0 = oldest day, last index = today.
 */
export function energyDayIsoDates(
  dayCount: number,
  referenceDateIso?: string,
  deviceDateTime?: string,
): string[] {
  if (dayCount <= 0) return [];
  let today: Date | null = null;
  if (referenceDateIso) {
    const p = /^(\d{4})-(\d{2})-(\d{2})$/.exec(referenceDateIso);
    if (p) {
      today = new Date(Number(p[1]), Number(p[2]) - 1, Number(p[3]));
    }
  }
  if (!today && deviceDateTime) {
    today = parseDeviceDateTime(deviceDateTime);
  }
  if (!today) return [];
  const out: string[] = [];
  for (let i = 0; i < dayCount; i++) {
    const daysAgo = dayCount - 1 - i;
    const d = new Date(today);
    d.setDate(d.getDate() - daysAgo);
    out.push(toIsoDateLocal(d));
  }
  return out;
}
