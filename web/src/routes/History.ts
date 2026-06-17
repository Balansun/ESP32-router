import type { RouteCtx } from "../router";
import { h } from "../utils/dom";
import { api } from "../api/client";
import { poll, deviceInfo, localePref } from "../state/store";
import { buildChart } from "../components/Chart";
import { getStrings } from "../i18n";
import { routeCleanup } from "../utils/routeLifecycle";
import { openDialog } from "../components/Dialog";
import { toast } from "../components/Toast";
import { go } from "../router";
import type { HistoryEnergyDaily, HistoryPower } from "../api/types";
import { isProbeTemperatureReading } from "../utils/format";
import {
  fetchLiveTemperatureTelemetry,
  historySeriesHasProbeTemp,
  resolveLiveTemperatureC,
  temperatureSensorAvailable,
} from "../api/temperatureTelemetry";
import { powerChartSeriesSpecs } from "../utils/chartChannelColors";
import {
  energyDayIsoDates,
  historyDailyDayCount,
  historyEnergyDaySpan,
} from "../utils/historyEnergy";
import { buildHistoryDailyImportCsv } from "../utils/historyDailyCsv";
import { hasTriac } from "../utils/firmwareCaps";
import {
  buildPowerTimeAxisHours,
  buildPowerTimeAxisSeconds,
  formatPowerAxisHours,
  formatPowerAxisSeconds,
  padSeries,
  powerHasSignal,
  powerHistoryWindowHours,
} from "../utils/historyPower";
import { buildHistoryRangePicker } from "../components/HistoryRangePicker";
import {
  formatHistoryChartMeta,
  historyChartTitle,
  historyMetricTabLabels,
  historyPowerPointCount,
  historyRangeOption,
  historyRangeOptions,
  parseHistoryPowerWindow,
  type HistoryPowerWindow,
} from "../utils/historyChartMeta";
import { buildPageHeader } from "../components/ui/pageHeader";
import { pmqttBindingsMissing } from "../utils/pmqttBindings";

type PowerHistoryWindow = HistoryPowerWindow;

function histMaxPointsCap(): number {
  return deviceInfo.get()?.capabilities?.hist_max_points ?? 200;
}

function defaultPowerWindow(): PowerHistoryWindow {
  const w = deviceInfo.get()?.capabilities?.history_power_window;
  if (w === "24h" || w === "48h") return w;
  return "48h";
}

function isoDateShiftDays(isoDate: string, deltaDays: number): string | null {
  const m = /^(\d{4})-(\d{2})-(\d{2})$/.exec(isoDate);
  if (!m) return null;
  const d = new Date(Number(m[1]), Number(m[2]) - 1, Number(m[3]));
  d.setDate(d.getDate() + deltaDays);
  return `${d.getFullYear()}-${String(d.getMonth() + 1).padStart(2, "0")}-${String(
    d.getDate(),
  ).padStart(2, "0")}`;
}

function resolveProbeTempVisible(
  power: HistoryPower | null,
  liveTempC: number | undefined,
  sensorAvailable: boolean,
): boolean {
  if (isProbeTemperatureReading(liveTempC)) return true;
  if (sensorAvailable) return true;
  if (power && isProbeTemperatureReading(power.temperature_now_c)) return true;
  if (power && historySeriesHasProbeTemp(power.temperature_series_c || [])) return true;
  return false;
}

function valueAt(arr: number[] | undefined, idx: number): number {
  const v = arr?.[idx];
  return typeof v === "number" && Number.isFinite(v) ? v : 0;
}

function formatKwh2(wh: number): string {
  return `${(wh / 1000).toFixed(2)} kWh`;
}

function toKwh(wh: number): number {
  return Math.round((wh / 1000) * 100) / 100;
}

export function mountHistory(ctx: RouteCtx): () => void {
  const { outlet, signal } = ctx;
  const T = getStrings();

  let powerWindow: PowerHistoryWindow = defaultPowerWindow();
  let lastPower: HistoryPower | null = null;
  let lastEnergy: HistoryEnergyDaily | null = null;
  let measurementSourceOk: boolean | null = null;
  let liveTempC: number | undefined;
  let tempSensorAvailable = false;
  let deviceDateTime: string | undefined;
  let tempTabVisible = false;
  let restartPowerPoll: () => void = () => {};

  const metricTabs = historyMetricTabLabels(T.history);

  const powerTitleEl = h("h3", { class: "card__title" }, historyChartTitle(T.history, "power"));
  const powerMetaEl = h("p", { class: "card__sub", "aria-live": "polite" });
  const tempTitleEl = h("h3", { class: "card__title" }, historyChartTitle(T.history, "temp"));
  const tempMetaEl = h("p", { class: "card__sub", "aria-live": "polite" });

  const pmqttBindingsBanner = h("p", {
    class: "banner banner--warn",
    role: "alert",
    hidden: true,
  });
  const noDataBanner = h("p", {
    class: "banner banner--warn",
    role: "status",
    hidden: true,
  });
  const settingsLink = h(
    "button",
    {
      type: "button",
      class: "btn btn--ghost",
      style: "margin-top:8px;",
      onClick: () => void go("/settings/metering"),
    },
    T.history.openSettings,
  );
  noDataBanner.append(
    h("span", {}, T.history.noDataYet),
    h("br"),
    settingsLink,
  );

  const powerChart = buildChart({
    title: "",
    yLabel: "W",
    y2Label: "VA",
    height: 260,
    series: powerChartSeriesSpecs({
      houseActive: T.history.legendHouse,
      triacActive: T.history.legendTriac,
      houseApparent: `${T.history.legendHouse} S`,
      triacApparent: `${T.history.legendTriac} S`,
    }),
    data: [[0], [0], [0], [0], [0]] as unknown as [
      number[],
      number[],
      number[],
      number[],
      number[],
    ],
    xFormat: (v) => formatPowerAxisHours(v),
  });

  const tempChart = buildChart({
    title: "",
    yLabel: "°C",
    height: 220,
    intLegend: false,
    series: [{ label: T.home.temperature, color: "var(--c-temp)", unit: "°C" }],
    data: [[0], [0]] as unknown as [number[], number[]],
    xFormat: (h) => formatPowerAxisHours(h),
  });
  const energyChart = buildChart({
    title: T.history.chart1yEnergyTitle,
    yLabel: "kWh",
    height: 180,
    intLegend: false,
    series: [
      { label: T.history.energySeriesCh1Import, color: "var(--c-chart-house-p)", unit: "kWh" },
      { label: T.history.energySeriesCh1Export, color: "var(--c-chart-house-s)", unit: "kWh" },
      { label: T.history.energySeriesCh2Import, color: "var(--c-chart-triac-p)", unit: "kWh" },
      { label: T.history.energySeriesCh2Export, color: "var(--c-chart-triac-s)", unit: "kWh" },
    ],
    data: [[0], [0], [0], [0], [0]] as unknown as [
      number[],
      number[],
      number[],
      number[],
      number[],
    ],
    xFormat: (idx) => {
      const i = Math.max(0, Math.floor(idx));
      const d = lastEnergy?.day_dates_iso?.[i];
      return d ? d.slice(5) : String(i + 1);
    },
  });

  const energyTableBody = h("tbody", {});

  const tabs = ["pw", "temp", "year"] as const;
  type Tab = (typeof tabs)[number];
  let activeTab: Tab = "pw";

  const tabsEl = h("div", {
    class: "tabs history-tabs",
    role: "tablist",
    "aria-label": T.history.tabsAria,
  });
  const panelsRoot = h("div", { class: "history-tabpanels" });
  const panes: Record<Tab, HTMLElement> = {
    pw: h(
      "section",
      {
        class: "card history-tabpanel",
        role: "tabpanel",
        id: "history-panel-pw",
      },
      h("div", { class: "card__head" }, powerTitleEl),
      powerMetaEl,
      noDataBanner,
      powerChart.el,
    ),
    temp: h(
      "section",
      {
        class: "card history-tabpanel",
        role: "tabpanel",
        id: "history-panel-temp",
      },
      h("div", { class: "card__head" }, tempTitleEl),
      tempMetaEl,
      tempChart.el,
    ),
    year: h(
      "section",
      {
        class: "card history-tabpanel",
        role: "tabpanel",
        id: "history-panel-year",
      },
      energyChart.el,
      h("h3", { class: "card__sub" }, T.history.energyTableTitle),
      h(
        "div",
        { class: "table-wrap history-energy-table-wrap" },
        h(
          "table",
          { class: "table" },
          h(
            "thead",
            {},
            h(
              "tr",
              {},
              h("th", {}, T.history.colDay),
              h("th", { class: "num" }, T.history.colCh1ImportWh),
              h("th", { class: "num" }, T.history.colCh1ExportWh),
              h("th", { class: "num" }, T.history.colCh2ImportWh),
              h("th", { class: "num" }, T.history.colCh2ExportWh),
            ),
          ),
          energyTableBody,
        ),
      ),
    ),
  };
  const labels: Record<Tab, string> = {
    pw: metricTabs.power,
    temp: metricTabs.temp,
    year: metricTabs.energy,
  };
  const panelIds: Record<Tab, string> = {
    pw: "history-panel-pw",
    temp: "history-panel-temp",
    year: "history-panel-year",
  };

  const buttons: Record<Tab, HTMLButtonElement> = {} as never;
  for (const tab of tabs) {
    const btn = h(
      "button",
      {
        type: "button",
        class: "tabs__btn",
        role: "tab",
        id: `history-tab-${tab}`,
        "aria-controls": panelIds[tab],
        "aria-selected": tab === "pw" ? "true" : "false",
        onClick: () => selectTab(tab),
      },
      labels[tab],
    );
    buttons[tab] = btn;
    tabsEl.append(btn);
  }
  buttons.temp.hidden = true;

  const rangePicker = buildHistoryRangePicker({
    name: "history-power-window",
    ariaLabel: T.history.rangeAria,
    value: powerWindow,
    options: historyRangeOptions(T.history).map((o) => ({
      id: o.id,
      label: o.shortLabel,
    })),
    onChange: (value) => {
      powerWindow = parseHistoryPowerWindow(value);
      syncChartHeaders(lastPower);
      void loadPower();
      restartPowerPoll();
    },
  });
  const rangePickerEl = rangePicker.el;

  function chartLocale(): string {
    return localePref.get() === "fr" ? "fr-FR" : "en-US";
  }

  function syncChartHeaders(power: HistoryPower | null) {
    powerTitleEl.textContent = historyChartTitle(T.history, "power");
    tempTitleEl.textContent = historyChartTitle(T.history, "temp");
    const rangeOpt = historyRangeOption(T.history, powerWindow);
    const samplePeriodS = power?.sample_period_s ?? rangeOpt.samplePeriodS;
    const pointCount = power ? historyPowerPointCount(power) : null;
    const meta = formatHistoryChartMeta(
      T.history,
      powerWindow,
      samplePeriodS,
      pointCount,
      chartLocale(),
    );
    powerMetaEl.textContent = meta;
    tempMetaEl.textContent = meta;
  }
  syncChartHeaders(null);

  function syncRangePickerVisibility() {
    rangePickerEl.hidden = activeTab === "year";
  }

  for (const tab of tabs) {
    panelsRoot.append(panes[tab]);
    panes[tab].hidden = tab !== "pw";
  }

  function setTempTabVisible(visible: boolean) {
    tempTabVisible = visible;
    buttons.temp.hidden = !visible;
    if (!visible && activeTab === "temp") selectTab("pw");
  }

  function selectTab(tab: Tab) {
    if (tab === "temp" && !tempTabVisible) tab = "pw";
    activeTab = tab;
    for (const t of tabs) {
      const isActive = t === tab;
      buttons[t].setAttribute("aria-selected", isActive ? "true" : "false");
      panes[t].hidden = !isActive;
    }
    syncRangePickerVisibility();
    queueMicrotask(() => {
      if (tab === "pw") powerChart.resize();
      else if (tab === "temp") tempChart.resize();
      else energyChart.resize();
    });
  }

  function syncTempTabVisibility() {
    setTempTabVisible(resolveProbeTempVisible(lastPower, liveTempC, tempSensorAvailable));
  }

  function syncPowerChartLabels() {
    const d = deviceInfo.get();
    const house = d?.probe_house_name || T.history.legendHouse;
    const triac = d?.probe_second_name || T.history.legendTriac;
    powerChart.setSeriesLabels([house, triac, `${house} S`, `${triac} S`]);
  }
  syncPowerChartLabels();

  function updateNoDataBanner(j: HistoryPower | null) {
    const show =
      measurementSourceOk === false ||
      (j !== null && !powerHasSignal(j.house_active_w || [], j.triac_active_w || []));
    noDataBanner.hidden = !show;
  }

  function applyPower(j: HistoryPower) {
    lastPower = j;
    syncChartHeaders(j);
    const is10m = powerWindow === "10m";
    const m = j.house_active_w || [];
    const tr = hasTriac() ? j.triac_active_w || [] : [];
    const vaM = j.house_apparent_va || [];
    const vaT = hasTriac() ? j.triac_apparent_va || [] : [];
    const n = Math.max(m.length, tr.length, vaM.length, vaT.length, 1);
    const periodS = j.sample_period_s || (is10m ? 2 : 300);
    const windowHours = powerHistoryWindowHours(powerWindow);
    const xs = is10m
      ? buildPowerTimeAxisSeconds(n, periodS)
      : buildPowerTimeAxisHours(n, windowHours);

    powerChart.setXFormat((v) =>
      is10m ? formatPowerAxisSeconds(v) : formatPowerAxisHours(v),
    );
    powerChart.setData([
      xs,
      padSeries(m, n),
      padSeries(tr, n),
      padSeries(vaM, n),
      padSeries(vaT, n),
    ] as unknown as [number[], number[], number[], number[], number[]]);

    updateNoDataBanner(j);
    syncTempTabVisibility();

    const t = j.temperature_series_c || [];
    const tempLabel = deviceInfo.get()?.temperature_label || T.home.temperature;
    tempChart.setSeriesLabels([tempLabel]);
    if (tempTabVisible && historySeriesHasProbeTemp(t)) {
      const is10mTemp = powerWindow === "10m";
      const xst = is10mTemp
        ? buildPowerTimeAxisSeconds(t.length, periodS)
        : buildPowerTimeAxisHours(t.length, windowHours);
      tempChart.setXFormat((v) =>
        is10mTemp ? formatPowerAxisSeconds(v) : formatPowerAxisHours(v),
      );
      tempChart.setData([xst, t] as unknown as [number[], number[]]);
    }
  }

  function applyEnergy(j: HistoryEnergyDaily) {
    lastEnergy = j;
    const dayCount = historyDailyDayCount(j);
    const iso =
      j.day_dates_iso?.length === dayCount
        ? j.day_dates_iso
        : energyDayIsoDates(
            dayCount,
            j.reference_date_iso,
            deviceDateTime,
          );
    const spanDays = historyEnergyDaySpan(
      j,
      deviceInfo.get()?.capabilities?.history_days_retained,
    );
    const chartStart = Math.max(0, dayCount - spanDays);
    const chartCount = dayCount - chartStart;
    const xs = Array.from({ length: chartCount }, (_, i) => i);
    const ch1ImportSeries = Array.from({ length: chartCount }, (_, i) =>
      toKwh(valueAt(j.ch1_import_wh_per_day, chartStart + i)),
    );
    const ch1ExportSeries = Array.from({ length: chartCount }, (_, i) =>
      toKwh(valueAt(j.ch1_export_wh_per_day, chartStart + i)),
    );
    const ch2ImportSeries = Array.from({ length: chartCount }, (_, i) =>
      toKwh(valueAt(j.ch2_import_wh_per_day, chartStart + i)),
    );
    const ch2ExportSeries = Array.from({ length: chartCount }, (_, i) =>
      toKwh(valueAt(j.ch2_export_wh_per_day, chartStart + i)),
    );
    energyChart.setData([
      xs,
      ch1ImportSeries,
      ch1ExportSeries,
      ch2ImportSeries,
      ch2ExportSeries,
    ] as unknown as [number[], number[], number[], number[], number[]]);
    energyTableBody.replaceChildren();
    const tableStart = chartStart;
    for (let i = dayCount - 1; i >= tableStart; i--) {
      const dayLabel = iso[i] ?? "—";
      const ch1Import = formatKwh2(valueAt(j.ch1_import_wh_per_day, i));
      const ch1Export = formatKwh2(valueAt(j.ch1_export_wh_per_day, i));
      const ch2Import = formatKwh2(valueAt(j.ch2_import_wh_per_day, i));
      const ch2Export = formatKwh2(valueAt(j.ch2_export_wh_per_day, i));
      energyTableBody.append(
        h(
          "tr",
          {},
          h("td", {}, dayLabel),
          h("td", { class: "num" }, ch1Import),
          h("td", { class: "num" }, ch1Export),
          h("td", { class: "num" }, ch2Import),
          h("td", { class: "num" }, ch2Export),
        ),
      );
    }
  }

  function downloadCsv() {
    if (!lastEnergy) {
      toast(T.history.loadError, "error");
      return;
    }
    const csv = buildHistoryDailyImportCsv(lastEnergy, deviceDateTime);
    const blob = new Blob([csv], { type: "text/csv;charset=utf-8" });
    const a = h("a", {
      href: URL.createObjectURL(blob),
      download: "balansun-history-daily.csv",
    }) as HTMLAnchorElement;
    a.click();
    URL.revokeObjectURL(a.href);
  }

  function confirmReset() {
    openDialog({
      title: T.history.resetHistory,
      body: h("p", {}, T.history.resetHistoryConfirm),
      actions: [
        { label: T.cancel, kind: "ghost", onClick: () => {} },
        {
          label: T.reset,
          kind: "danger",
          onClick: async () => {
            await api.resetHistory({ signal });
            toast(T.history.resetDone, "success");
            await loadPower();
            await loadEnergy();
          },
        },
      ],
    });
  }

  const exportBtn = h(
    "button",
    { type: "button", class: "btn btn--ghost", onClick: downloadCsv },
    T.history.exportCsv,
  );
  const resetBtn = h(
    "button",
    { type: "button", class: "btn btn--danger", onClick: confirmReset },
    T.history.resetHistory,
  );

  async function loadPower() {
    powerWindow = parseHistoryPowerWindow(rangePicker.getValue());
    rangePicker.setValue(powerWindow);
    syncChartHeaders(lastPower);
    try {
      const j = await api.getHistoryPower(powerWindow, histMaxPointsCap(), { signal });
      applyPower(j);
    } catch {
      toast(T.history.loadError, "error");
    }
  }

  async function loadEnergy() {
    try {
      const nowIso =
        lastEnergy?.reference_date_iso ??
        (deviceDateTime ? energyDayIsoDates(1, undefined, deviceDateTime)[0] : undefined) ??
        new Date().toISOString().slice(0, 10);
      const span = historyEnergyDaySpan(
        lastEnergy,
        deviceInfo.get()?.capabilities?.history_days_retained,
      );
      const fromIso = isoDateShiftDays(nowIso, -(span - 1)) ?? nowIso;
      const j = await api.getHistoryEnergyDailyRangeAll(fromIso, nowIso, { signal });
      applyEnergy(j);
    } catch {
      toast(T.history.loadError, "error");
    }
  }

  outlet.append(
    buildPageHeader({
      title: T.history.title,
      actions: [exportBtn, resetBtn],
    }),
    pmqttBindingsBanner,
    tabsEl,
    rangePickerEl,
    panelsRoot,
  );

  const unsubDevice = deviceInfo.subscribe(() => syncPowerChartLabels());

  return routeCleanup(signal, (scope) => {
    scope.onUnmount(() => {
      unsubDevice();
      powerChart.destroy();
      tempChart.destroy();
      energyChart.destroy();
    });

    async function refreshLiveContext() {
      try {
        const m = await api.getMeasurements({ signal }).catch(() => null);
        if (m) {
          liveTempC = resolveLiveTemperatureC(m);
          tempSensorAvailable = temperatureSensorAvailable(m);
          deviceDateTime = m.date_valid ? m.date : undefined;
        } else {
          const live = await fetchLiveTemperatureTelemetry({ signal });
          liveTempC = resolveLiveTemperatureC(live);
          tempSensorAvailable = temperatureSensorAvailable(live);
        }
        if (liveTempC == null) {
          const st = await api.getState({ signal }).catch(() => null);
          if (st) {
            liveTempC = resolveLiveTemperatureC(st);
            tempSensorAvailable =
              tempSensorAvailable || temperatureSensorAvailable(st);
          }
        }
        if (lastEnergy) applyEnergy(lastEnergy);
      } catch {
        liveTempC = undefined;
        tempSensorAvailable = false;
      }
      syncTempTabVisibility();
    }

    void (async () => {
      try {
        const [health, cfgRes] = await Promise.all([
          api.getHealth({ signal }),
          api.getConfig({ signal }),
        ]);
        measurementSourceOk = health.source_ok;
        pmqttBindingsBanner.hidden = !pmqttBindingsMissing(cfgRes.config);
        if (!pmqttBindingsBanner.hidden) {
          pmqttBindingsBanner.textContent = T.home.pmqttBindingsMissing;
        }
        updateNoDataBanner(lastPower);
      } catch {
        measurementSourceOk = null;
        pmqttBindingsBanner.hidden = true;
      }
    })();

    void refreshLiveContext();
    void loadPower();
    void loadEnergy();

    const powerPollMs = () => (powerWindow === "10m" ? 12_000 : 60_000);

    let powerPollTimer: ReturnType<typeof poll> | null = null;
    restartPowerPoll = () => {
      powerPollTimer?.stop();
      powerPollTimer = poll(
        async () => {
          await loadPower();
        },
        powerPollMs(),
        30_000,
        { immediate: false },
      );
      scope.trackPoll(powerPollTimer);
    };
    restartPowerPoll();
    syncRangePickerVisibility();

    scope.trackPoll(
      poll(
        async () => {
          await refreshLiveContext();
        },
        60_000,
        30_000,
        { immediate: false },
      ),
    );

    scope.trackPoll(
      poll(
        async () => {
          await loadEnergy();
        },
        300_000,
        30_000,
        { immediate: false },
      ),
    );
  });
}
