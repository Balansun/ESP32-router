import { describe, expect, it, vi } from "vitest";
import { en } from "../src/i18n/locales/en";
import {
  buildTimezoneCountryField,
  countryIsoForTz,
  tzForCountryIso,
} from "../src/components/TimezoneCountryField";

import { INSTALL_COUNTRIES } from "../src/data/install-countries";

describe("TimezoneCountryField", () => {
  it("countryIsoForTz returns prefer iso when listed in INSTALL_COUNTRIES", () => {
    const iso = INSTALL_COUNTRIES.find((c) => c.iso2 === "DE")!.iso2;
    expect(countryIsoForTz("", iso)).toBe(iso);
  });

  it("tzForCountryIso resolves known countries and UTC for ZZ", () => {
    expect(tzForCountryIso("FR")).toBe("Europe/Paris");
    expect(tzForCountryIso("ZZ")).toBe("UTC");
    expect(tzForCountryIso("ZW")).toBe("Africa/Harare");
    expect(tzForCountryIso("XX")).toBe("UTC");
  });

  it("countryIsoForTz with non-empty tz skips empty-tz prefer branch", () => {
    expect(countryIsoForTz("Europe/Paris")).toBe("FR");
    expect(countryIsoForTz("Europe/Berlin", "US")).toBe("DE");
    expect(countryIsoForTz("  Europe/Paris  ")).toBe("FR");
  });

  it("countryIsoForTz with empty tz uses prefer iso when valid", () => {
    expect(countryIsoForTz("", "DE")).toBe("DE");
  });

  it("countryIsoForTz maps tz back to iso", () => {
    expect(countryIsoForTz("Europe/Paris", "FR")).toBe("FR");
    expect(countryIsoForTz("", "DE")).toBe("DE");
    expect(countryIsoForTz("Unknown/Tz")).toBe("ZZ");
  });

  it("writeTz with empty string falls back to FR default", () => {
    const field = buildTimezoneCountryField(en, "en", "Europe/Paris", "FR");
    field.writeTz("");
    expect(field.readTz()).toBe(tzForCountryIso("FR"));
  });

  it("countryIsoForTz returns ZZ when tz unknown and no prefer match", () => {
    expect(countryIsoForTz("Pacific/Fake", "XX")).toBe("ZZ");
  });

  it("builds with empty initial tz using FR default", () => {
    const field = buildTimezoneCountryField(en, "en", "", undefined);
    expect(field.readTz()).toBe(tzForCountryIso("FR"));
  });

  it("countryIsoForTz maps by suggestedTimeTz when prefer does not match", () => {
    expect(countryIsoForTz("Europe/Berlin", "US")).toBe("DE");
  });

  it("countryIsoForTz uses prefer iso when tz empty and prefer is valid", () => {
    expect(countryIsoForTz("", "DE")).toBe("DE");
    expect(countryIsoForTz("", "FR")).toBe("FR");
    expect(countryIsoForTz("", "XX")).toBe("FR");
    expect(countryIsoForTz("", "  ")).toBe("FR");
  });

  it("countryIsoForTz maps tz via INSTALL_COUNTRIES lookup", () => {
    expect(countryIsoForTz("America/Argentina/Buenos_Aires")).toBe("AR");
  });

  it("onChange fires when country select changes", () => {
    const onChange = vi.fn();
    const field = buildTimezoneCountryField(en, "en", "Europe/Paris", "FR", onChange);
    document.body.append(field.el);
    const select = field.el.querySelector("#time_tz_country") as HTMLSelectElement;
    select.value = "DE";
    select.dispatchEvent(new Event("change", { bubbles: true }));
    expect(onChange).toHaveBeenCalled();
  });

  it("buildTimezoneCountryField reads and writes tz", () => {
    const onChange = vi.fn();
    const field = buildTimezoneCountryField(en, "en", "Europe/Paris", "FR", onChange);
    expect(field.readTz()).toBe("Europe/Paris");
    field.writeTz("Europe/Berlin");
    expect(field.readTz()).toBe("Europe/Berlin");
    const select = field.el.querySelector("select") as HTMLSelectElement;
    select.value = "DE";
    select.dispatchEvent(new Event("change", { bubbles: true }));
    expect(onChange).toHaveBeenCalled();
    expect(field.readTz()).toBe(tzForCountryIso("DE"));
  });
});
