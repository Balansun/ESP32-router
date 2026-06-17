import type { AppStrings } from "../i18n/locales/en";

export type HistoryPowerWindow = "10m" | "24h" | "48h";

export function parseHistoryPowerWindow(value: string): HistoryPowerWindow {
  if (value === "10m") return "10m";
  if (value === "24h") return "24h";
  return "48h";
}

export interface HistoryRangeOption {
  id: HistoryPowerWindow;
  shortLabel: string;
  longLabel: string;
  samplePeriodS: number;
}

export function historyRangeOptions(T: AppStrings["history"]): HistoryRangeOption[] {
  return [
    { id: "10m", shortLabel: T.range10m, longLabel: T.windowLast10m, samplePeriodS: 2 },
    { id: "24h", shortLabel: T.range24h, longLabel: T.windowLast24h, samplePeriodS: 300 },
    { id: "48h", shortLabel: T.range48h, longLabel: T.windowLast48h, samplePeriodS: 300 },
  ];
}

export function historyRangeOption(
  T: AppStrings["history"],
  window: HistoryPowerWindow,
): HistoryRangeOption {
  const found = historyRangeOptions(T).find((o) => o.id === window);
  return found ?? historyRangeOptions(T)[1];
}

export function historyMetricTabLabels(T: AppStrings["history"]) {
  return { power: T.tabPower, temp: T.tabTemp, energy: T.tab1yEnergy };
}

export type HistoryChartMetric = "power" | "temp";

export function historyChartTitle(T: AppStrings["history"], metric: HistoryChartMetric): string {
  return metric === "power" ? T.chartPowerTitle : T.chartTempTitle;
}

function windowLongLabel(T: AppStrings["history"], window: HistoryPowerWindow): string {
  switch (window) {
    case "10m":
      return T.windowLast10m;
    case "24h":
      return T.windowLast24h;
    default:
      return T.windowLast48h;
  }
}

function formatResolution(T: AppStrings["history"], samplePeriodS: number): string {
  return samplePeriodS <= 2 ? T.resolution2s : T.resolution5min;
}

function formatPointCount(points: number, locale: string): string {
  return new Intl.NumberFormat(locale).format(points);
}

/** Subtitle under power/temperature charts (window, resolution, optional point count). */
export function formatHistoryChartMeta(
  T: AppStrings["history"],
  window: HistoryPowerWindow,
  samplePeriodS: number,
  pointCount: number | null,
  locale = "en-US",
): string {
  const windowLabel = windowLongLabel(T, window);
  const resolution = formatResolution(T, samplePeriodS);
  if (pointCount === null) {
    return T.metaTemplateLoading.replace("{window}", windowLabel).replace("{resolution}", resolution);
  }
  const points = formatPointCount(pointCount, locale);
  return T.metaTemplate
    .replace("{window}", windowLabel)
    .replace("{resolution}", resolution)
    .replace("{points}", points);
}

export function historyPowerPointCount(power: {
  house_active_w?: number[];
  triac_active_w?: number[];
  house_apparent_va?: number[];
  triac_apparent_va?: number[];
  temperature_series_c?: number[];
}): number {
  const lengths = [
    power.house_active_w?.length ?? 0,
    power.triac_active_w?.length ?? 0,
    power.house_apparent_va?.length ?? 0,
    power.triac_apparent_va?.length ?? 0,
    power.temperature_series_c?.length ?? 0,
  ];
  return Math.max(...lengths, 0);
}
