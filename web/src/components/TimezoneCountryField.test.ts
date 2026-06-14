import { describe, expect, it } from "vitest";
import { countryIsoForTz, tzForCountryIso } from "./TimezoneCountryField";

describe("TimezoneCountryField helpers", () => {
  it("tzForCountryIso returns country suggestion or UTC", () => {
    expect(tzForCountryIso("FR")).toBe("Europe/Paris");
    expect(tzForCountryIso("ZZ")).toBe("UTC");
  });

  it("countryIsoForTz handles empty and non-empty tz", () => {
    expect(countryIsoForTz("", "DE")).toBe("DE");
    expect(countryIsoForTz("Europe/Paris", "FR")).toBe("FR");
    expect(countryIsoForTz("Europe/Berlin", "US")).toBe("DE");
    expect(countryIsoForTz("  Europe/Paris  ")).toBe("FR");
    expect(countryIsoForTz("")).toBe("FR");
    expect(countryIsoForTz("", null as unknown as string)).toBe("FR");
  });
});
