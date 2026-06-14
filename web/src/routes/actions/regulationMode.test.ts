import { describe, expect, it } from "vitest";
import {
  isActionRegulationEnabled,
  MODE_DECOUPE_ONOFF,
  MODE_INACTIF,
  setActionRegulationEnabled,
} from "./regulationMode";

describe("regulationMode", () => {
  it("isActionRegulationEnabled is false for inactive", () => {
    expect(isActionRegulationEnabled(undefined)).toBe(false);
    expect(isActionRegulationEnabled(MODE_INACTIF)).toBe(false);
  });

  it("isActionRegulationEnabled is true for active modes", () => {
    expect(isActionRegulationEnabled(MODE_DECOUPE_ONOFF)).toBe(true);
    expect(isActionRegulationEnabled(3)).toBe(true);
  });

  it("setActionRegulationEnabled turns off to inactive", () => {
    expect(setActionRegulationEnabled(MODE_DECOUPE_ONOFF, false)).toBe(MODE_INACTIF);
  });

  it("setActionRegulationEnabled from inactive enables on/off mode", () => {
    expect(setActionRegulationEnabled(undefined, true)).toBe(MODE_DECOUPE_ONOFF);
    expect(setActionRegulationEnabled(MODE_INACTIF, true)).toBe(MODE_DECOUPE_ONOFF);
  });

  it("setActionRegulationEnabled preserves expert mode when enabling", () => {
    expect(setActionRegulationEnabled(5, true)).toBe(5);
  });
});
