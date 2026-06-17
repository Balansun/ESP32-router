import { describe, expect, it } from "vitest";
import {
  apparentHasSignal,
  buildPowerTimeAxisHours,
  buildPowerTimeAxisSeconds,
  formatPowerAxisHours,
  formatPowerAxisSeconds,
  maxAbs,
  padSeries,
  powerHasSignal,
  powerHistoryWindowHours,
} from "../src/utils/historyPower";
import {
  energyDayIsoDates as energyDates,
  historyDailyDayCount,
  parseDeviceDateTime,
  toIsoDateLocal,
} from "../src/utils/historyEnergy";
import {
  HISTORY_DAILY_CSV_HEADER,
  buildHistoryDailyImportCsv,
} from "../src/utils/historyDailyCsv";
import type { HistoryEnergyDaily } from "../src/api/types";
import {
  formatHistoryChartMeta,
  historyChartTitle,
  historyMetricTabLabels,
  historyPowerPointCount,
  historyRangeOptions,
  parseHistoryPowerWindow,
} from "../src/utils/historyChartMeta";
import { en } from "../src/i18n/locales/en";

describe("historyPower", () => {
  it("powerHasSignal detects non-flat series", () => {
    expect(powerHasSignal([0, 0], [0])).toBe(false);
    expect(powerHasSignal([0, 42], [])).toBe(true);
    expect(powerHasSignal([], [5])).toBe(true);
  });

  it("buildPowerTimeAxisSeconds ends at 0", () => {
    const xs = buildPowerTimeAxisSeconds(3, 2);
    expect(xs[0]).toBe(-4);
    expect(xs[1]).toBe(-2);
    expect(xs[2]).toBe(0);
  });

  it("padSeries extends with zeros", () => {
    expect(padSeries([10], 3)).toEqual([10, 0, 0]);
  });

  it("formatPowerAxisSeconds", () => {
    expect(formatPowerAxisSeconds(-125)).toBe("2:05");
    expect(formatPowerAxisSeconds(-60)).toBe("1 min");
    expect(formatPowerAxisSeconds(-30)).toBe("30s");
    expect(formatPowerAxisSeconds(0)).toBe("0s");
  });

  it("maxAbs ignores non-finite values", () => {
    expect(maxAbs([NaN, -3, Infinity])).toBe(3);
    expect(maxAbs([])).toBe(0);
  });

  it("padSeries truncates when longer than target", () => {
    expect(padSeries([1, 2, 3, 4], 2)).toEqual([1, 2]);
  });

  it("apparentHasSignal", () => {
    expect(apparentHasSignal([], [])).toBe(false);
    expect(apparentHasSignal([500], [])).toBe(true);
  });

  it("buildPowerTimeAxisSeconds uses default period when invalid", () => {
    expect(buildPowerTimeAxisSeconds(2, 0)).toEqual([-2, 0]);
  });

  it("buildPowerTimeAxisHours and formatPowerAxisHours", () => {
    const xs = buildPowerTimeAxisHours(2, 24);
    expect(xs[0]).toBeCloseTo(-24, 5);
    expect(xs[1]).toBeCloseTo(0, 5);
    expect(formatPowerAxisHours(0)).toBe("0");
    expect(formatPowerAxisHours(-1)).toBe("1 h");
    expect(formatPowerAxisHours(-1.25)).toBe("1h15");
  });

  it("buildPowerTimeAxisHours spans selected window regardless of point count", () => {
    const xs24 = buildPowerTimeAxisHours(200, 24);
    expect(xs24[0]).toBeCloseTo(-24, 5);
    expect(xs24[xs24.length - 1]).toBe(0);

    const xs48 = buildPowerTimeAxisHours(600, 48);
    expect(xs48[0]).toBeCloseTo(-48, 5);
    expect(xs48[xs48.length - 1]).toBe(0);
  });

  it("powerHistoryWindowHours maps API window labels", () => {
    expect(powerHistoryWindowHours("24h")).toBe(24);
    expect(powerHistoryWindowHours("48h")).toBe(48);
    expect(powerHistoryWindowHours("10m")).toBe(48);
  });
});

describe("historyChartMeta", () => {
  const T = en.history;

  it("parseHistoryPowerWindow maps select values", () => {
    expect(parseHistoryPowerWindow("10m")).toBe("10m");
    expect(parseHistoryPowerWindow("24h")).toBe("24h");
    expect(parseHistoryPowerWindow("48h")).toBe("48h");
    expect(parseHistoryPowerWindow("other")).toBe("48h");
  });

  it("historyMetricTabLabels are static", () => {
    expect(historyMetricTabLabels(T)).toEqual({
      power: T.tabPower,
      temp: T.tabTemp,
      energy: T.tab1yEnergy,
    });
  });

  it("historyChartTitle returns metric-only labels", () => {
    expect(historyChartTitle(T, "power")).toBe(T.chartPowerTitle);
    expect(historyChartTitle(T, "temp")).toBe(T.chartTempTitle);
  });

  it("formatHistoryChartMeta builds subtitle with points", () => {
    expect(formatHistoryChartMeta(T, "24h", 300, 192, "en-US")).toBe(
      "Last 24 hours · 5 min bins · 192 points",
    );
  });

  it("formatHistoryChartMeta omits points while loading", () => {
    expect(formatHistoryChartMeta(T, "48h", 300, null, "en-US")).toBe(
      "Last 48 hours · 5 min bins",
    );
  });

  it("historyPowerPointCount uses longest series", () => {
    expect(
      historyPowerPointCount({
        house_active_w: [1, 2, 3],
        triac_active_w: [1, 2],
      }),
    ).toBe(3);
  });

  it("historyRangeOptions expose short labels and sample periods", () => {
    expect(historyRangeOptions(T).map((o) => o.id)).toEqual(["10m", "24h", "48h"]);
    expect(historyRangeOptions(T)[0].samplePeriodS).toBe(2);
    expect(historyRangeOptions(T)[1].samplePeriodS).toBe(300);
  });
});

describe("historyEnergy", () => {
  it("parseDeviceDateTime", () => {
    const d = parseDeviceDateTime("22/05/2026 14:30:00");
    expect(d).not.toBeNull();
    expect(toIsoDateLocal(d!)).toBe("2026-05-22");
  });

  it("energyDayIsoDates from reference", () => {
    expect(energyDates(3, "2026-05-22")).toEqual([
      "2026-05-20",
      "2026-05-21",
      "2026-05-22",
    ]);
  });

  it("parseDeviceDateTime rejects invalid calendar dates", () => {
    expect(parseDeviceDateTime("31/02/2026")).toBeNull();
    expect(parseDeviceDateTime("not-a-date")).toBeNull();
  });

  it("energyDayIsoDates from device clock when reference missing", () => {
    expect(energyDates(2, undefined, "27/05/2026")).toEqual([
      "2026-05-26",
      "2026-05-27",
    ]);
    expect(energyDates(0, "2026-05-22")).toEqual([]);
  });

  it("historyDailyDayCount prefers dates then CH arrays", () => {
    const withDates: HistoryEnergyDaily = {
      delta_wh_per_day: [1],
      import_wh_per_day: [1],
      export_wh_per_day: [0],
      ch1_import_wh_per_day: [100],
      ch1_export_wh_per_day: [0],
      ch2_import_wh_per_day: [50],
      ch2_export_wh_per_day: [0],
      day_dates_iso: ["2026-05-01", "2026-05-02"],
      count: 2,
      total_count: 2,
      has_more: false,
      offset: 0,
      limit: 10,
    };
    expect(historyDailyDayCount(withDates)).toBe(2);

    const metricsOnly: HistoryEnergyDaily = {
      ...withDates,
      day_dates_iso: [],
      ch1_import_wh_per_day: [10, 20],
      ch2_import_wh_per_day: [5],
      delta_wh_per_day: [3],
    };
    expect(historyDailyDayCount(metricsOnly)).toBe(2);
  });
});

describe("historyDailyCsv", () => {
  const sample: HistoryEnergyDaily = {
    delta_wh_per_day: [100],
    import_wh_per_day: [100],
    export_wh_per_day: [0],
    ch1_import_wh_per_day: [17571.4, 16847],
    ch1_export_wh_per_day: [2, 537],
    ch2_import_wh_per_day: [1227],
    ch2_export_wh_per_day: [Number.NaN],
    day_dates_iso: ["2026-02-27", "2026-02-28"],
    reference_date_iso: "2026-02-28",
    count: 2,
    total_count: 2,
    has_more: false,
    offset: 0,
    limit: 10,
  };

  it("buildHistoryDailyImportCsv uses day_dates_iso and rounds Wh", () => {
    const csv = buildHistoryDailyImportCsv(sample);
    expect(csv.startsWith(`${HISTORY_DAILY_CSV_HEADER}\n`)).toBe(true);
    expect(csv).toContain("2026-02-27,17571,2,1227,0");
    expect(csv).toContain("2026-02-28,16847,537,0,0");
    expect(csv.endsWith("\n")).toBe(true);
  });

  it("buildHistoryDailyImportCsv derives dates when day_dates_iso absent", () => {
    const csv = buildHistoryDailyImportCsv({
      ...sample,
      day_dates_iso: [],
    });
    expect(csv.split("\n").filter(Boolean).length).toBe(3);
    expect(csv).toContain("2026-02-27,17571,2,1227,0");
    expect(csv).toContain("2026-02-28,16847,537,0,0");
  });
});
