import { describe, expect, it, vi } from "vitest";
import {
  INSTALL_COUNTRIES,
  countryDisplayName,
  lookupInstallCountry,
  resolveInstallLookupKey,
} from "../src/data/install-countries";

describe("install-countries", () => {
  it("resolveInstallLookupKey normalizes iso and variant", () => {
    expect(resolveInstallLookupKey("fr", "")).toBe("FR");
    expect(resolveInstallLookupKey("de", "BY")).toContain("DE");
  });

  it("lookupInstallCountry finds FR", () => {
    const row = lookupInstallCountry("FR");
    expect(row?.lookupKey).toBe("FR");
    expect(row?.suggestedTimeTz).toBeTruthy();
  });

  it("countryDisplayName returns localized label", () => {
    expect(countryDisplayName("FR", "en")).toContain("France");
    expect(countryDisplayName("FR", "fr")).toBeTruthy();
  });

  it("resolveInstallLookupKey handles empty and ZZ", () => {
    expect(resolveInstallLookupKey("", "")).toBe("ZZ");
    expect(resolveInstallLookupKey("zz", "")).toBe("ZZ");
  });

  it("resolveInstallLookupKey resolves every plain country iso", () => {
    for (const c of INSTALL_COUNTRIES) {
      if (c.iso2 === "ZZ") continue;
      expect(resolveInstallLookupKey(c.iso2.toLowerCase(), "")).toBe(c.lookupKey);
    }
  });

  it("resolveInstallLookupKey matches splitVariants lookupKey", () => {
    expect(resolveInstallLookupKey("br", "127")).toBe("BR-127");
    expect(lookupInstallCountry("BR", "127")?.lookupKey).toBe("BR-127");
  });

  it("lookupInstallCountry resolves split variant BR-127", () => {
    const row = lookupInstallCountry("BR", "127");
    expect(row?.lookupKey).toBe("BR-127");
    expect(row?.defaultNominalV).toBe(127);
  });

  it("lookupInstallCountry returns undefined for unknown", () => {
    expect(lookupInstallCountry("XX", "")).toBeUndefined();
  });

  it("resolveInstallLookupKey accepts hyphenated variant id", () => {
    expect(resolveInstallLookupKey("br", "BR-220")).toBe("BR-220");
  });

  it("resolveInstallLookupKey matches Japan split variant lookupKey", () => {
    expect(resolveInstallLookupKey("jp", "JP-W")).toBe("JP-W");
    expect(resolveInstallLookupKey("jp", "W")).toBe("JP-W");
    expect(lookupInstallCountry("JP", "W")?.lookupKey).toBe("JP-W");
  });

  it("resolveInstallLookupKey falls back to iso when variant unknown", () => {
    expect(resolveInstallLookupKey("de", "nope")).toBe("DE");
  });

  it("countryDisplayName uses iso when Intl.of returns undefined", () => {
    vi.spyOn(Intl, "DisplayNames").mockImplementation(
      () =>
        ({
          of: () => undefined,
        }) as Intl.DisplayNames,
    );
    expect(countryDisplayName("FR", "en")).toBe("FR");
    vi.restoreAllMocks();
  });

  it("countryDisplayName falls back when Intl fails", () => {
    vi.spyOn(Intl, "DisplayNames").mockImplementation(() => {
      throw new Error("unsupported");
    });
    expect(countryDisplayName("DE", "xx")).toBe("DE");
    vi.restoreAllMocks();
  });
});
