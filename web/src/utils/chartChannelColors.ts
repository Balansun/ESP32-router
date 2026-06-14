import type { SeriesSpec } from "../components/Chart";

/** CSS variables for power history chart series (resolved to canvas colors in Chart.ts). */
export const CHART_POWER_CHANNELS = {
  houseActive: "var(--c-chart-house-p)",
  triacActive: "var(--c-chart-triac-p)",
  houseApparent: "var(--c-chart-house-s)",
  triacApparent: "var(--c-chart-triac-s)",
} as const;

export interface PowerChartSeriesLabels {
  houseActive: string;
  triacActive: string;
  houseApparent: string;
  triacApparent: string;
}

export function powerChartSeriesSpecs(
  labels: PowerChartSeriesLabels,
): SeriesSpec[] {
  return [
    {
      label: labels.houseActive,
      color: CHART_POWER_CHANNELS.houseActive,
      unit: "W",
      scale: "y",
    },
    {
      label: labels.triacActive,
      color: CHART_POWER_CHANNELS.triacActive,
      unit: "W",
      scale: "y",
    },
    {
      label: labels.houseApparent,
      color: CHART_POWER_CHANNELS.houseApparent,
      unit: "VA",
      scale: "y2",
    },
    {
      label: labels.triacApparent,
      color: CHART_POWER_CHANNELS.triacApparent,
      unit: "VA",
      scale: "y2",
    },
  ];
}
