import { localePref } from "../state/store";

function numberLocale(): string {
  return localePref.get() === "fr" ? "fr-FR" : "en-US";
}

function nfInt() {
  return new Intl.NumberFormat(numberLocale(), { maximumFractionDigits: 0 });
}
function nfOne() {
  return new Intl.NumberFormat(numberLocale(), {
    minimumFractionDigits: 1,
    maximumFractionDigits: 1,
  });
}
/** Up to one decimal; whole numbers omit “.0” (19 not 19.0, 19.1 unchanged). */
function nfTempC() {
  return new Intl.NumberFormat(numberLocale(), { maximumFractionDigits: 1 });
}
function nfTwo() {
  return new Intl.NumberFormat(numberLocale(), {
    minimumFractionDigits: 2,
    maximumFractionDigits: 2,
  });
}

export function fmtInt(n: number | null | undefined): string {
  if (n == null || !isFinite(n)) return "—";
  return nfInt().format(Math.round(n));
}

export function fmt1(n: number | null | undefined): string {
  if (n == null || !isFinite(n)) return "—";
  return nfOne().format(n);
}

export function fmt2(n: number | null | undefined): string {
  if (n == null || !isFinite(n)) return "—";
  return nfTwo().format(n);
}

/** RMS / line voltage (V) — two decimal places. */
export function fmtVolts(v: number | null | undefined): string {
  return fmt2(v);
}

/** RMS current (A) — two decimal places. */
export function fmtAmps(a: number | null | undefined): string {
  return fmt2(a);
}

/** Instantaneous power (W or VA) — whole units (matches firmware int storage). */
export function fmtPowerW(w: number | null | undefined): string {
  return fmtInt(w);
}

export function fmtEnergyWh(wh: number | null | undefined): {
  value: string;
  unit: "Wh" | "kWh" | "MWh";
} {
  if (wh == null || !isFinite(wh)) return { value: "—", unit: "Wh" };
  const abs = Math.abs(wh);
  if (abs >= 1_000_000) return { value: fmt2(wh / 1_000_000), unit: "MWh" };
  if (abs >= 1000) return { value: fmt2(wh / 1000), unit: "kWh" };
  if (Number.isInteger(wh)) return { value: fmtInt(wh), unit: "Wh" };
  return { value: fmt2(wh), unit: "Wh" };
}

export function fmtPercent(n: number | null | undefined): string {
  if (n == null || !isFinite(n)) return "—";
  return `${fmtInt(n)} %`;
}

/** Firmware sentinel -127 means no DS18B20 reading. */
export function isProbeTemperatureReading(c: number | null | undefined): boolean {
  return typeof c === "number" && Number.isFinite(c) && c > -100;
}

export function fmtTempCValue(n: number | null | undefined): string {
  if (n == null || !isFinite(n)) return "—";
  return nfTempC().format(n);
}

export function fmtTempC(n: number | null | undefined): string {
  if (!isProbeTemperatureReading(n)) return "—";
  return `${fmtTempCValue(n as number)} °C`;
}

export function fmtBytes(n: number | null | undefined): string {
  if (n == null || !isFinite(n) || n < 0) return "—";
  if (n < 1024) return `${Math.round(n)} B`;
  if (n < 1024 * 1024) return `${fmt1(n / 1024)} KB`;
  return `${fmt2(n / (1024 * 1024))} MB`;
}

export {
  fmtHourMinFrom2400,
  from2400ToMinutes,
  minutesTo2400,
} from "./time2400";

export function isValidIp(s: string): boolean {
  if (!s) return false;
  const parts = s.split(".");
  if (parts.length !== 4) return false;
  for (const p of parts) {
    if (!/^\d+$/.test(p)) return false;
    const n = Number(p);
    if (n < 0 || n > 255) return false;
  }
  return true;
}
