import { describe, expect, it, vi } from "vitest";
import type { ActionPeriod } from "../api/types";
import { buildPeriodScheduleList } from "./PeriodScheduleList";

const periods: ActionPeriod[] = [
  {
    mode: "off",
    hour_end: 800,
    power_min_w: 0,
    power_max_w: 0,
    temp_inf_c: 150,
    temp_sup_c: 150,
  },
  {
    mode: "power",
    hour_end: 2400,
    power_min_w: 100,
    power_max_w: 100,
    temp_inf_c: 150,
    temp_sup_c: 150,
  },
];

describe("PeriodScheduleList DOM", () => {
  it("renders period rows with time windows", () => {
    const list = buildPeriodScheduleList({ onPickPeriod: () => {} });
    list.setPeriods(periods, false);
    const items = list.el.querySelectorAll(".period-list__item");
    expect(items.length).toBe(2);
    expect(list.el.querySelector(".period-list__time")?.textContent).toContain("00:00");
  });

  it("renders on mode without power summary", () => {
    const onPeriods: ActionPeriod[] = [
      {
        mode: "on",
        hour_end: 2400,
        power_min_w: 0,
        power_max_w: 0,
        temp_inf_c: 150,
        temp_sup_c: 150,
      },
    ];
    const list = buildPeriodScheduleList({ onPickPeriod: () => {} });
    list.setPeriods(onPeriods, false);
    expect(list.el.querySelector(".period-list__summary")).toBeNull();
  });

  it("renders triac power styling when isTriac", () => {
    const list = buildPeriodScheduleList({ onPickPeriod: () => {} });
    list.setPeriods(periods, true);
    const modePills = list.el.querySelectorAll(".period-list__mode");
    expect(modePills.length).toBeGreaterThanOrEqual(2);
    expect(modePills[1]?.getAttribute("style")).toContain("var(--c-warn)");
  });
});
