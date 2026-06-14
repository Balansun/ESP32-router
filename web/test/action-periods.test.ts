import { describe, expect, it } from "vitest";
import type { ActionPeriod } from "../src/api/types";
import {
  clampPeriodEndMinutes,
  periodStart2400,
  periodWindowLabel,
  minutesToTimeInputValue,
  timeInputValueToMinutes,
} from "../src/utils/actionPeriods";

describe("actionPeriods", () => {
  const periods: ActionPeriod[] = [
    { mode: "off", hour_end: 800, power_min_w: 0, power_max_w: 0, temp_inf_c: 150, temp_sup_c: 150 },
    { mode: "power", hour_end: 1800, power_min_w: 100, power_max_w: 100, temp_inf_c: 150, temp_sup_c: 150 },
    { mode: "off", hour_end: 2400, power_min_w: 0, power_max_w: 0, temp_inf_c: 150, temp_sup_c: 150 },
  ];

  it("periodWindowLabel formats a range", () => {
    expect(periodWindowLabel(0, 800)).toBe("00:00–08:00");
  });

  it("periodStart2400 uses previous hour_end", () => {
    expect(periodStart2400(periods, 0)).toBe(0);
    expect(periodStart2400(periods, 1)).toBe(800);
  });

  it("clampPeriodEndMinutes respects neighbours", () => {
    expect(clampPeriodEndMinutes(periods, 0, 500)).toBe(500);
    expect(clampPeriodEndMinutes(periods, 0, 900)).toBe(900);
    expect(clampPeriodEndMinutes(periods, 0, 0)).toBe(0);
    expect(clampPeriodEndMinutes(periods, 1, 2000)).toBe(1440);
  });

  it("timeInputValueToMinutes parses HH:MM", () => {
    expect(timeInputValueToMinutes("08:30")).toBe(510);
    expect(timeInputValueToMinutes("24:00")).toBe(1440);
    expect(timeInputValueToMinutes("bad")).toBe(0);
    expect(timeInputValueToMinutes("12")).toBe(0);
    expect(timeInputValueToMinutes("08:")).toBe(8 * 60);
    expect(timeInputValueToMinutes("08:xx")).toBe(8 * 60);
    expect(timeInputValueToMinutes("aa:30")).toBe(30);
  });

  it("minutesToTimeInputValue formats padded time", () => {
    expect(minutesToTimeInputValue(510)).toBe("08:30");
    expect(minutesToTimeInputValue(2000)).toBe("24:00");
  });
});
