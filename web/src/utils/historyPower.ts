/** Shared helpers for GET /api/v1/history/power UI. */

export function maxAbs(values: number[]): number {
  let m = 0;
  for (const v of values) {
    if (typeof v === "number" && Number.isFinite(v)) {
      const a = Math.abs(v);
      if (a > m) m = a;
    }
  }
  return m;
}

export function padSeries(values: number[], length: number): number[] {
  if (values.length >= length) return values.slice(0, length);
  return [...values, ...Array(length - values.length).fill(0)];
}

export function powerHasSignal(
  house: number[],
  triac: number[] = [],
): boolean {
  if (house.length === 0 && triac.length === 0) return false;
  return maxAbs(house) >= 1 || maxAbs(triac) >= 1;
}

export function apparentHasSignal(
  houseVa: number[] = [],
  triacVa: number[] = [],
): boolean {
  return maxAbs(houseVa) >= 1 || maxAbs(triacVa) >= 1;
}

/** X axis: seconds before now (negative), oldest point left. */
export function buildPowerTimeAxisSeconds(
  pointCount: number,
  samplePeriodS: number,
): number[] {
  const n = Math.max(1, pointCount);
  const period = samplePeriodS > 0 ? samplePeriodS : 2;
  return Array.from({ length: n }, (_, i) => {
    const v = -((n - 1 - i) * period);
    return v === 0 ? 0 : v;
  });
}

/** Map GET /history/power `window` to chart span in hours. */
export function powerHistoryWindowHours(window: string): 24 | 48 {
  if (window === "24h") return 24;
  return 48;
}

/** X axis for 24h/48h windows: hours before now (negative), evenly spaced. */
export function buildPowerTimeAxisHours(
  pointCount: number,
  windowHours: number,
): number[] {
  const n = Math.max(1, pointCount);
  const span = windowHours > 0 ? windowHours : 48;
  if (n === 1) return [0];
  return Array.from({ length: n }, (_, i) => {
    const v = -(span * (n - 1 - i)) / (n - 1);
    return v === 0 ? 0 : v;
  });
}

export function formatPowerAxisSeconds(seconds: number): string {
  const abs = Math.abs(Math.round(seconds));
  if (abs < 60) return `${abs}s`;
  const m = Math.floor(abs / 60);
  const s = abs % 60;
  if (s === 0) return `${m} min`;
  return `${m}:${String(s).padStart(2, "0")}`;
}

export function formatPowerAxisHours(hours: number): string {
  const abs = Math.abs(hours);
  if (abs < 1 / 60) return "0";
  const totalMin = Math.round(abs * 60);
  if (totalMin < 60) return `${totalMin} min`;
  const h = Math.floor(totalMin / 60);
  const m = totalMin % 60;
  if (m === 0) return `${h} h`;
  return `${h}h${String(m).padStart(2, "0")}`;
}
