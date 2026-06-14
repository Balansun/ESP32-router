import { describe, expect, it } from "vitest";
import type { HistoryEnergyDaily } from "../src/api/types";
import { historyEnergyDaySpan } from "../src/utils/historyEnergy";

describe("historyEnergyDaySpan", () => {
  it("prefers days_retained from energy payload", () => {
    const energy = {
      days_retained: 7,
      days_capacity: 7,
    } as HistoryEnergyDaily;
    expect(historyEnergyDaySpan(energy)).toBe(7);
  });

  it("falls back to capabilities when energy has no retention fields", () => {
    expect(historyEnergyDaySpan(null, 30)).toBe(30);
  });

  it("defaults to 90 for legacy mocks", () => {
    expect(historyEnergyDaySpan(null)).toBe(90);
  });
});
