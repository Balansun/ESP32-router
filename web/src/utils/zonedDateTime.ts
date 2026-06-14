/** Wall-clock ↔ Unix epoch (UTC) for a given IANA timezone (no external deps). */

import { lookupInstallCountry } from "../data/install-countries";

function tzForCountryIso(iso2: string, variant?: string): string {
  const row = lookupInstallCountry(iso2, variant);
  return row?.suggestedTimeTz ?? "UTC";
}

const DATETIME_LOCAL_RE = /^(\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2})$/;

/** True when `tz` is accepted by Intl (IANA id). POSIX strings (e.g. CET-1CEST) fail. */
export function isValidIanaTimeZone(tz: string): boolean {
  const t = (tz || "").trim();
  if (!t) return false;
  try {
    Intl.DateTimeFormat(undefined, { timeZone: t });
    return true;
  } catch {
    return false;
  }
}

/** Device TZ if IANA, else install-country suggestion, else UTC. */
export function resolveIntlTimeZone(
  deviceTz: string,
  installCountry: string,
  variant?: string,
): string {
  const d = (deviceTz || "").trim();
  if (d.length > 0 && isValidIanaTimeZone(d)) {
    return d;
  }
  const country = (installCountry || "").trim();
  const fallback = tzForCountryIso(country.length > 0 ? country : "FR", variant);
  if (isValidIanaTimeZone(fallback)) {
    return fallback;
  }
  return "UTC";
}

function normalizeIanaTimeZone(timeZone: string): string {
  const t = timeZone.trim();
  if (t.length > 0 && isValidIanaTimeZone(t)) {
    return t;
  }
  return "UTC";
}

function partsInTimeZone(epochMs: number, timeZone: string) {
  const parts = new Intl.DateTimeFormat("en-US", {
    timeZone,
    year: "numeric",
    month: "2-digit",
    day: "2-digit",
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
    hourCycle: "h23",
  }).formatToParts(new Date(epochMs));
  const pick = (type: Intl.DateTimeFormatPartTypes) =>
    Number(parts.find((p) => p.type === type)?.value ?? "0");
  return {
    year: pick("year"),
    month: pick("month"),
    day: pick("day"),
    hour: pick("hour"),
    minute: pick("minute"),
  };
}

/** Epoch seconds → `datetime-local` value (`YYYY-MM-DDTHH:mm`) in `timeZone`. */
export function epochSecToDatetimeLocalValue(epochSec: number, timeZone: string): string {
  if (!Number.isFinite(epochSec) || epochSec <= 0) return "";
  const tz = normalizeIanaTimeZone(timeZone);
  try {
    const formatted = new Intl.DateTimeFormat("sv-SE", {
      timeZone: tz,
      year: "numeric",
      month: "2-digit",
      day: "2-digit",
      hour: "2-digit",
      minute: "2-digit",
      hourCycle: "h23",
    }).format(new Date(epochSec * 1000));
    return formatted.replace(" ", "T");
  } catch {
    /* v8 ignore next */
    return "";
  }
}

/** `datetime-local` wall time in `timeZone` → epoch seconds UTC, or `null` if empty/invalid. */
export function datetimeLocalValueToEpochSec(value: string, timeZone: string): number | null {
  const trimmed = value.trim();
  if (!trimmed) return null;

  const m = DATETIME_LOCAL_RE.exec(trimmed);
  if (!m) return null;

  const year = Number(m[1]);
  const month = Number(m[2]);
  const day = Number(m[3]);
  const hour = Number(m[4]);
  const minute = Number(m[5]);

  if (
    !Number.isFinite(year) ||
    month < 1 ||
    month > 12 ||
    day < 1 ||
    day > 31 ||
    hour > 23 ||
    minute > 59
  ) {
    return null;
  }

  const tz = normalizeIanaTimeZone(timeZone);
  let utcMs = Date.UTC(year, month - 1, day, hour, minute, 0);

  try {
    for (let i = 0; i < 4; i++) {
      const got = partsInTimeZone(utcMs, tz);
      const wantAsUtc = Date.UTC(year, month - 1, day, hour, minute, 0);
      const gotAsUtc = Date.UTC(got.year, got.month - 1, got.day, got.hour, got.minute, 0);
      utcMs += wantAsUtc - gotAsUtc;
    }

    const check = partsInTimeZone(utcMs, tz);
    if (
      check.year !== year ||
      check.month !== month ||
      check.day !== day ||
      check.hour !== hour ||
      check.minute !== minute
    ) {
      return null;
    }

    return Math.round(utcMs / 1000);
  } catch {
    /* v8 ignore next */
    return null;
  }
}
