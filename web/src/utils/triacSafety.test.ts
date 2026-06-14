import { describe, expect, it } from "vitest";
import type { ActionPeriod } from "../api/types";
import { triacPeriodsHaveTemperatureGating } from "./triacSafety";

function period(overrides: Partial<ActionPeriod>): ActionPeriod {
  return {
    mode: "power",
    hour_end: 2400,
    power_min_w: 0,
    power_max_w: 100,
    temp_inf_c: 150,
    temp_sup_c: 150,
    ...overrides,
  };
}

describe("triacPeriodsHaveTemperatureGating", () => {
  it("returns false when all periods use disabled temp bounds", () => {
    expect(triacPeriodsHaveTemperatureGating([period({})])).toBe(false);
  });

  it("returns true when power period has lower bound enabled", () => {
    expect(
      triacPeriodsHaveTemperatureGating([period({ temp_inf_c: 50 })]),
    ).toBe(true);
  });

  it("returns true for on mode with upper bound enabled", () => {
    expect(
      triacPeriodsHaveTemperatureGating([
        period({ mode: "on", temp_sup_c: 60 }),
      ]),
    ).toBe(true);
  });

  it("ignores off periods", () => {
    expect(
      triacPeriodsHaveTemperatureGating([
        period({ mode: "off", temp_inf_c: 40 }),
      ]),
    ).toBe(false);
  });

});
