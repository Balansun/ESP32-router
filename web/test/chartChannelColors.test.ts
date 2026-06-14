/** @vitest-environment happy-dom */
import { describe, expect, it } from "vitest";
import { resolveChartColor } from "../src/components/Chart";
import {
  CHART_POWER_CHANNELS,
  powerChartSeriesSpecs,
} from "../src/utils/chartChannelColors";

describe("CHART_POWER_CHANNELS", () => {
  it("defines four distinct CSS variables", () => {
    const values = Object.values(CHART_POWER_CHANNELS);
    expect(new Set(values).size).toBe(4);
    for (const v of values) {
      expect(v).toMatch(/^var\(--c-chart-/);
    }
  });
});

describe("powerChartSeriesSpecs", () => {
  it("returns four series with channel colors and scales", () => {
    const specs = powerChartSeriesSpecs({
      houseActive: "House P",
      triacActive: "Triac P",
      houseApparent: "House S",
      triacApparent: "Triac S",
    });
    expect(specs).toHaveLength(4);
    expect(specs[0].color).toBe(CHART_POWER_CHANNELS.houseActive);
    expect(specs[1].color).toBe(CHART_POWER_CHANNELS.triacActive);
    expect(specs[2].color).toBe(CHART_POWER_CHANNELS.houseApparent);
    expect(specs[3].color).toBe(CHART_POWER_CHANNELS.triacApparent);
    expect(specs[0].scale).toBe("y");
    expect(specs[2].scale).toBe("y2");
  });
});

describe("resolveChartColor", () => {
  it("resolves var(--token) via computed style", () => {
    const root = document.documentElement;
    root.style.setProperty("--test-chart-color", "#ff5500");
    expect(resolveChartColor("var(--test-chart-color)")).toBe("#ff5500");
    root.style.removeProperty("--test-chart-color");
  });

  it("passes through literal colors", () => {
    expect(resolveChartColor("oklch(60% 0.2 30)")).toBe("oklch(60% 0.2 30)");
  });
});
