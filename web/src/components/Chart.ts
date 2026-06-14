import uPlot, { type Options as UplotOptions, type AlignedData } from "uplot";
import { h } from "../utils/dom";
import { localePref } from "../state/store";

export interface SeriesSpec {
  label: string;
  color: string;
  unit?: string;
  width?: number;
  /** Primary (W) or secondary (VA, etc.). Default `y`. */
  scale?: "y" | "y2";
}

export interface ChartOpts {
  title?: string;
  yLabel?: string;
  y2Label?: string;
  xFormat?: (v: number) => string;
  series: SeriesSpec[];
  data: AlignedData;
  height?: number;
  area?: boolean;
  intLegend?: boolean;
}

export interface ChartHandle {
  el: HTMLElement;
  setData(data: AlignedData): void;
  setSeriesLabels(labels: string[]): void;
  setXFormat(fn: (v: number) => string): void;
  /** Re-measure width after tab panel becomes visible. */
  resize(): void;
  destroy(): void;
}

const css = (v: string) =>
  getComputedStyle(document.documentElement).getPropertyValue(v).trim() || v;

/** uPlot canvas cannot parse `var(--token)`; resolve to computed color. */
export function resolveChartColor(color: string): string {
  const m = /^var\(\s*(--[\w-]+)\s*\)$/.exec(color.trim());
  if (m) return css(m[1]);
  return color;
}

function chartLocale(): string {
  return localePref.get() === "fr" ? "fr-FR" : "en-US";
}

function flatYMax(yLabel: string | undefined): number {
  if (yLabel === "°C") return 40;
  if (yLabel === "Wh") return 5000;
  if (yLabel === "VA") return 500;
  return 500;
}

function collectYValues(
  data: AlignedData,
  seriesSpecs: SeriesSpec[],
  isSeriesVisible?: (seriesIndex: number) => boolean,
): {
  y: number[];
  y2: number[];
} {
  const y: number[] = [];
  const y2: number[] = [];
  for (let si = 1; si < data.length; si++) {
    if (isSeriesVisible && !isSeriesVisible(si)) continue;
    const spec = seriesSpecs[si - 1];
    const bucket = spec?.scale === "y2" ? y2 : y;
    const series = data[si];
    if (!Array.isArray(series)) continue;
    for (const v of series) {
      if (typeof v === "number" && Number.isFinite(v)) bucket.push(v);
    }
  }
  return { y, y2 };
}

function scaleRange(
  values: number[],
  yLabel: string | undefined,
): [number, number] | null {
  if (values.length === 0) return [0, flatYMax(yLabel)];
  const min = Math.min(...values);
  const max = Math.max(...values);
  if (max - min < 1) return [0, flatYMax(yLabel)];
  const pad = Math.max(1, (max - min) * 0.05);
  return [min - pad, max + pad];
}

function yScalesForData(
  data: AlignedData,
  seriesSpecs: SeriesSpec[],
  yLabel: string | undefined,
  y2Label: string | undefined,
  isSeriesVisible?: (seriesIndex: number) => boolean,
): UplotOptions["scales"] {
  const { y, y2 } = collectYValues(data, seriesSpecs, isSeriesVisible);
  const hasY2 = seriesSpecs.some((s) => s.scale === "y2");
  const x = { time: false };
  const scales: UplotOptions["scales"] = { x };
  const yr = scaleRange(y, yLabel);
  if (yr) scales.y = { range: yr };
  if (hasY2) {
    const y2r = scaleRange(y2, y2Label ?? "VA");
    if (y2r) scales.y2 = { range: y2r };
  }
  return scales;
}

function isTemperatureChart(opts: ChartOpts): boolean {
  if (opts.yLabel === "°C" || opts.y2Label === "°C") return true;
  return opts.series.some((s) => s.unit === "°C");
}

export function buildChart(opts: ChartOpts): ChartHandle {
  const wrap = h("div", { class: "chart-wrap" });
  const titleEl = opts.title
    ? h("h3", { class: "card__title" }, opts.title)
    : null;
  const canvas = h("div", { class: "chart-canvas" });

  let seriesLabels = opts.series.map((s) => s.label);
  let chart: uPlot | null = null;
  let lastData: AlignedData = opts.data;
  let xFormat = opts.xFormat;
  const useInt = opts.intLegend !== false && !isTemperatureChart(opts);
  const hasY2 = () => opts.series.some((s) => s.scale === "y2");

  function numberFormatters() {
    const loc = chartLocale();
    return {
      intFmt: new Intl.NumberFormat(loc, { maximumFractionDigits: 0 }),
      oneFmt: new Intl.NumberFormat(loc, { maximumFractionDigits: 1 }),
    };
  }

  function formatYValue(raw: number | null, useInteger: boolean): string {
    if (raw == null) return "—";
    const { intFmt, oneFmt } = numberFormatters();
    return useInteger ? intFmt.format(raw) : oneFmt.format(raw);
  }

  function makeOptions(width: number): UplotOptions {
    const dual = hasY2();
    const seriesDefs: UplotOptions["series"] = [
      { label: "x" },
      ...opts.series.map((s, i) => {
        const stroke = resolveChartColor(s.color);
        return {
        label: seriesLabels[i] ?? s.label,
        scale: s.scale === "y2" ? "y2" : "y",
        stroke,
        width: s.width ?? 2,
        fill: opts.area
          ? `color-mix(in oklab, ${stroke} 20%, transparent)`
          : undefined,
        spanGaps: true,
        value: (_: uPlot, raw: number | null) => {
          if (raw == null) return "—";
          const unit =
            s.unit ??
            (s.scale === "y2" ? opts.y2Label : opts.yLabel) ??
            "";
          const n = formatYValue(raw, useInt);
          return unit ? `${n} ${unit}` : n;
        },
      };
      }),
    ];

    const axes: UplotOptions["axes"] = [
      {
        stroke: css("--c-fg-muted"),
        grid: { stroke: css("--grid-line"), width: 1 },
        ticks: { stroke: css("--grid-line"), width: 1 },
        values: xFormat
          ? (_, splits) => splits.map((v) => xFormat!(v))
          : undefined,
      },
      {
        scale: "y",
        side: 3,
        stroke: css("--c-fg-muted"),
        grid: { stroke: css("--grid-line"), width: 1 },
        ticks: { stroke: css("--grid-line"), width: 1 },
        size: 56,
        values: (_, splits) =>
          splits.map((v) => formatYValue(v, useInt)),
      },
    ];

    if (dual) {
      axes.push({
        scale: "y2",
        side: 1,
        stroke: css("--c-fg-muted"),
        grid: { show: false },
        ticks: { stroke: css("--c-fg-muted"), width: 1 },
        size: 48,
        values: (_, splits) =>
          splits.map((v) => formatYValue(v, useInt)),
      });
    }

    return {
      width,
      height: opts.height ?? 240,
      legend: { show: true, live: true },
      cursor: {
        drag: { x: false, y: false },
        focus: { prox: 24 },
        points: { size: 7 },
      },
      scales: yScalesForData(lastData, opts.series, opts.yLabel, opts.y2Label),
      axes,
      series: seriesDefs,
      hooks: {
        // Recompute y-axis range when user toggles series visibility.
        setSeries: [() => applyScales()],
      },
    };
  }

  function applyScales() {
    if (!chart) return;
    const scales = yScalesForData(
      lastData,
      opts.series,
      opts.yLabel,
      opts.y2Label,
      (seriesIndex) => chart?.series?.[seriesIndex]?.show !== false,
    );
    const yr = scales?.y?.range;
    if (Array.isArray(yr) && yr.length >= 2) {
      chart.setScale("y", { min: yr[0] as number, max: yr[1] as number });
    }
    const y2r = scales?.y2?.range;
    if (Array.isArray(y2r) && y2r.length >= 2) {
      chart.setScale("y2", { min: y2r[0] as number, max: y2r[1] as number });
    }
  }

  function ensure() {
    const w = canvas.clientWidth || wrap.clientWidth || 320;
    if (!chart) {
      chart = new uPlot(makeOptions(w), lastData, canvas);
    } else {
      chart.setSize({ width: w, height: opts.height ?? 240 });
    }
  }

  let ro: ResizeObserver | null = null;
  if ("ResizeObserver" in window) {
    ro = new ResizeObserver(() => ensure());
  }
  const srSummary = h("p", { class: "sr-only", "aria-live": "polite" });
  wrap.setAttribute("role", "img");
  if (titleEl) wrap.append(titleEl);
  wrap.append(canvas, srSummary);

  function updateSrSummary(data: AlignedData) {
    const parts: string[] = [];
    for (let si = 1; si < data.length && si - 1 < opts.series.length; si++) {
      const series = data[si];
      if (!Array.isArray(series) || series.length === 0) continue;
      const last = series[series.length - 1];
      if (typeof last !== "number" || !Number.isFinite(last)) continue;
      const spec = opts.series[si - 1];
      const unit =
        spec.unit ??
        (spec.scale === "y2" ? opts.y2Label : opts.yLabel) ??
        "";
      parts.push(`${spec.label}: ${formatYValue(last, useInt)}${unit ? ` ${unit}` : ""}`);
    }
    srSummary.textContent = parts.length ? parts.join(", ") : "";
    wrap.setAttribute(
      "aria-label",
      opts.title ? `${opts.title}. ${srSummary.textContent}` : srSummary.textContent,
    );
  }
  updateSrSummary(lastData);

  queueMicrotask(() => {
    ensure();
    if (ro && wrap.parentElement) ro.observe(wrap);
  });

  return {
    el: wrap,
    setData(data: AlignedData) {
      lastData = data;
      updateSrSummary(data);
      if (!chart) return;
      chart.setData(data);
      applyScales();
    },
    setSeriesLabels(labels: string[]) {
      seriesLabels = [...labels];
      if (!chart) return;
      const data = chart.data;
      const w = chart.width;
      chart.destroy();
      chart = new uPlot(makeOptions(w), data, canvas);
    },
    setXFormat(fn: (v: number) => string) {
      xFormat = fn;
      if (!chart) return;
      const data = chart.data;
      const w = chart.width;
      chart.destroy();
      chart = new uPlot(makeOptions(w), data, canvas);
    },
    resize() {
      ensure();
    },
    destroy() {
      ro?.disconnect();
      chart?.destroy();
      chart = null;
    },
  };
}
