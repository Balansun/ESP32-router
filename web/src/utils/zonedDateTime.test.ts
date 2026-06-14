import { afterEach, describe, expect, it, vi } from "vitest";
import * as installCountries from "../data/install-countries";
import {
  datetimeLocalValueToEpochSec,
  epochSecToDatetimeLocalValue,
  isValidIanaTimeZone,
  resolveIntlTimeZone,
} from "./zonedDateTime";

describe("zonedDateTime", () => {
  afterEach(() => {
    vi.restoreAllMocks();
  });

  it("rejects POSIX TZ strings for Intl", () => {
    const posix = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";
    expect(isValidIanaTimeZone(posix)).toBe(false);
    expect(resolveIntlTimeZone(posix, "FR")).toBe("Europe/Paris");
  });

  it("accepts IANA device TZ", () => {
    expect(resolveIntlTimeZone("Europe/Berlin", "FR")).toBe("Europe/Berlin");
    expect(resolveIntlTimeZone("America/New_York", "FR")).toBe("America/New_York");
    expect(isValidIanaTimeZone("Europe/Paris")).toBe(true);
    expect(epochSecToDatetimeLocalValue(1_700_000_000, "Europe/Paris")).toMatch(/^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}$/);
  });

  it("returns empty string for zero epoch", () => {
    expect(epochSecToDatetimeLocalValue(0, "Europe/Paris")).toBe("");
  });

  it("returns null for empty input", () => {
    expect(datetimeLocalValueToEpochSec("", "Europe/Paris")).toBeNull();
    expect(datetimeLocalValueToEpochSec("   ", "Europe/Paris")).toBeNull();
  });

  it("round-trips summer time in Europe/Paris", () => {
    const tz = "Europe/Paris";
    const local = "2026-09-01T08:00";
    const epoch = datetimeLocalValueToEpochSec(local, tz);
    expect(epoch).not.toBeNull();
    expect(epochSecToDatetimeLocalValue(epoch!, tz)).toBe(local);
    expect(epoch).toBe(Math.floor(Date.UTC(2026, 8, 1, 6, 0, 0) / 1000));
  });

  it("round-trips winter time in Europe/Paris", () => {
    const tz = "Europe/Paris";
    const local = "2026-01-15T08:00";
    const epoch = datetimeLocalValueToEpochSec(local, tz);
    expect(epoch).not.toBeNull();
    expect(epochSecToDatetimeLocalValue(epoch!, tz)).toBe(local);
    expect(epoch).toBe(Math.floor(Date.UTC(2026, 0, 15, 7, 0, 0) / 1000));
  });

  it("rejects malformed values", () => {
    expect(datetimeLocalValueToEpochSec("not-a-date", "UTC")).toBeNull();
    expect(datetimeLocalValueToEpochSec("2026-13-40T25:99", "UTC")).toBeNull();
    expect(datetimeLocalValueToEpochSec("2026-02-30T12:00", "UTC")).toBeNull();
  });

  it("isValidIanaTimeZone rejects empty", () => {
    expect(isValidIanaTimeZone("")).toBe(false);
  });

  it("resolveIntlTimeZone falls back for ZZ", () => {
    expect(resolveIntlTimeZone("", "ZZ")).toBe("UTC");
  });

  it("epochSecToDatetimeLocalValue returns empty when Intl throws", () => {
    const fmt = Intl.DateTimeFormat;
    vi.spyOn(Intl, "DateTimeFormat").mockImplementation(() => {
      throw new RangeError("bad tz");
    });
    expect(epochSecToDatetimeLocalValue(1_700_000_000, "Bad/Zone")).toBe("");
    Intl.DateTimeFormat = fmt;
  });

  it("resolveIntlTimeZone uses country suggestion when device TZ invalid", () => {
    expect(resolveIntlTimeZone("Bad/Zone", "DE")).toBe("Europe/Berlin");
  });

  it("resolveIntlTimeZone uses UTC for unknown country", () => {
    expect(resolveIntlTimeZone("", "ZZ")).toBe("UTC");
  });

  it("resolveIntlTimeZone defaults empty install country to FR", () => {
    expect(resolveIntlTimeZone("", "")).toBe("Europe/Paris");
  });

  it("normalizeIanaTimeZone accepts empty timeZone string", () => {
    expect(epochSecToDatetimeLocalValue(1_700_000_000, "")).toContain("T");
  });

  it("resolveIntlTimeZone uses UTC when country fallback is invalid IANA", () => {
    vi.spyOn(Intl, "DateTimeFormat").mockImplementation((locales, opts) => {
      if (opts && typeof opts === "object" && "timeZone" in opts && opts.timeZone === "UTC") {
        throw new RangeError("bad");
      }
      return new Intl.DateTimeFormat(locales, opts);
    });
    expect(resolveIntlTimeZone("", "ZZ")).toBe("UTC");
    vi.restoreAllMocks();
  });

  it("datetimeLocalValueToEpochSec returns null when conversion throws", () => {
    const fmt = Intl.DateTimeFormat;
    vi.spyOn(Intl, "DateTimeFormat").mockImplementation(() => {
      throw new RangeError("bad tz");
    });
    expect(datetimeLocalValueToEpochSec("2026-01-15T08:00", "Bad/Zone")).toBeNull();
    Intl.DateTimeFormat = fmt;
  });

  it("resolveIntlTimeZone returns UTC when country suggestion is not valid IANA", () => {
    vi.spyOn(installCountries, "lookupInstallCountry").mockReturnValue({
      suggestedTimeTz: "Not/A/Valid",
    } as ReturnType<typeof installCountries.lookupInstallCountry>);
    expect(resolveIntlTimeZone("", "DE")).toBe("UTC");
  });

  it("normalizeIanaTimeZone path uses valid device zone in epoch format", () => {
    const local = epochSecToDatetimeLocalValue(1_700_000_000, "Europe/Paris");
    expect(local).toContain("T");
    expect(epochSecToDatetimeLocalValue(1_700_000_000, "UTC")).toContain("T");
  });

  it("datetimeLocalValueToEpochSec tolerates missing Intl format parts", () => {
    vi.spyOn(Intl.DateTimeFormat.prototype, "formatToParts").mockReturnValue([
      { type: "year", value: "2026" },
      { type: "month", value: "01" },
      { type: "day", value: "15" },
      { type: "hour", value: "08" },
    ] as Intl.DateTimeFormatPart[]);
    expect(datetimeLocalValueToEpochSec("2026-01-15T08:00", "UTC")).not.toBeNull();
    vi.restoreAllMocks();
  });
});
