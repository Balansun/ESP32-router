/** Firmware HHMM×100 time encoding (0 = midnight, 2400 = end of day). */

export function fmtHourMinFrom2400(hhmm2400: number): string {
  const h = Math.floor(hhmm2400 / 100);
  const minRaw = hhmm2400 - 100 * h;
  const m = Math.floor((minRaw / 100) * 60);
  return `${String(h).padStart(2, "0")}:${String(m).padStart(2, "0")}`;
}

export function minutesTo2400(totalMin: number): number {
  totalMin = Math.max(0, Math.min(1440, Math.round(totalMin)));
  const h = Math.floor(totalMin / 60);
  const m = totalMin - 60 * h;
  return h * 100 + Math.round((m / 60) * 100);
}

export function from2400ToMinutes(v: number): number {
  const h = Math.floor(v / 100);
  const minRaw = v - 100 * h;
  const m = Math.round((minRaw / 100) * 60);
  return Math.max(0, Math.min(1440, h * 60 + m));
}
