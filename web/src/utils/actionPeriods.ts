import type { ActionPeriod } from "../api/types";
import { fmtHourMinFrom2400, from2400ToMinutes } from "./time2400";

export function periodWindowLabel(start2400: number, end2400: number): string {
  return `${fmtHourMinFrom2400(start2400)}–${fmtHourMinFrom2400(end2400)}`;
}

export function periodStart2400(periods: ActionPeriod[], index: number): number {
  return index > 0 ? periods[index - 1].hour_end : 0;
}

export function clampPeriodEndMinutes(
  periods: ActionPeriod[],
  periodIdx: number,
  minutes: number,
): number {
  const prev = periodIdx > 0 ? from2400ToMinutes(periods[periodIdx - 1].hour_end) : 0;
  const next =
    periodIdx < periods.length - 2
      ? from2400ToMinutes(periods[periodIdx + 1].hour_end)
      : 1440;
  return Math.max(prev, Math.min(next, minutes));
}

export function minutesToTimeInputValue(minutes: number): string {
  const m = Math.max(0, Math.min(1440, Math.round(minutes)));
  const h = Math.floor(m / 60);
  const min = m % 60;
  return `${String(h).padStart(2, "0")}:${String(min).padStart(2, "0")}`;
}

export function timeInputValueToMinutes(value: string): number {
  const parts = value.split(":");
  if (parts.length < 2) return 0;
  const h = parseInt(parts[0], 10) || 0;
  const min = parseInt(parts[1], 10) || 0;
  return Math.max(0, Math.min(1440, h * 60 + min));
}
