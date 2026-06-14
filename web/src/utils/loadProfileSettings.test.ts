import { describe, expect, it } from "vitest";
import { getStrings } from "../i18n";
import { loadProfileOptions, normalizeLoadProfile } from "./loadProfileSettings";

describe("loadProfileSettings", () => {
  it("normalizes load profile wire ids", () => {
    expect(normalizeLoadProfile("generic_triac")).toBe("generic_triac");
    expect(normalizeLoadProfile("water_heater_triac")).toBe("water_heater_triac");
    expect(normalizeLoadProfile(undefined)).toBe("water_heater_triac");
    expect(normalizeLoadProfile("unknown")).toBe("water_heater_triac");
  });

  it("builds localized load profile options", () => {
    const T = getStrings();
    const options = loadProfileOptions(T);
    expect(options).toHaveLength(2);
    expect(options[0].value).toBe("water_heater_triac");
    expect(options[1].value).toBe("generic_triac");
    expect(options[0].label).toBe(T.settings.loadProfiles.water_heater_triac);
  });
});
