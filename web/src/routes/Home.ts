import { h } from "../utils/dom";
import type { RouteCtx } from "../router";
import { api, ApiError } from "../api/client";
import type { ActionOverride, OverrideState } from "../api/types";
import { poll, deviceInfo } from "../state/store";
import { buildKpi } from "../components/KpiCard";
import { buildTariffBadge } from "../components/TariffBadge";
import { buildMainsFrequencyBanner } from "../components/MainsFrequencyBanner";
import { buildChart } from "../components/Chart";
import {
  buildOverrideControls,
  overrideMap,
} from "../components/ActionOverrideBar";
import { toast } from "../components/Toast";
import { fmtEnergyWh, fmtPercent, fmtPowerW, fmtTempC, isProbeTemperatureReading } from "../utils/format";
import {
  fetchLiveTemperatureTelemetry,
  historySeriesHasProbeTemp,
  resolveLiveTemperatureC,
  temperatureSensorAvailable,
} from "../api/temperatureTelemetry";
import type { HistoryPower } from "../api/types";
import { powerChartSeriesSpecs } from "../utils/chartChannelColors";
import { pmqttBindingsMissing } from "../utils/pmqttBindings";
import {
  buildPowerTimeAxisSeconds,
  formatPowerAxisSeconds,
  padSeries,
  powerHasSignal,
} from "../utils/historyPower";
import {
  confirmLegionellaTriacFull,
  triacPeriodsHaveTemperatureGating,
} from "../utils/triacSafety";
import { getStrings } from "../i18n";
import { routeCleanup } from "../utils/routeLifecycle";
import { buildCacsiRegulationBanner } from "../components/CacsiRegulationBanner";
import { buildLegionellaGatingBanner } from "../components/LegionellaGatingBanner";
import { openSourceSetupWizard } from "../components/SourceSetupWizard";
import { hasSurplusRegulation, hasTriac } from "../utils/firmwareCaps";
import { isSafetyLockoutActive } from "../utils/safetyLockout";
import { withBase } from "../paths";

export function mountHome(ctx: RouteCtx): () => void {
  const { outlet, signal } = ctx;
  const T = getStrings();

  const kImport = buildKpi({
    label: T.home.importLabel,
    variant: "import",
    unit: "W",
  });
  const kExport = buildKpi({
    label: T.home.exportLabel,
    variant: "export",
    unit: "W",
  });
  const kApparentImp = buildKpi({
    label: T.home.apparentLabel,
    variant: "apparent",
    unit: "VA",
  });
  const kEnergyImp = buildKpi({
    label: T.home.energyDayImport,
    variant: "energy",
  });
  const kEnergyExp = buildKpi({
    label: T.home.energyDayExport,
    variant: "energy",
  });
  const kTemp = buildKpi({
    label: T.home.temperature,
    variant: "temp",
  });

  const tariff = buildTariffBadge(T.home.tariff);
  const sourceHealthBadge = h("span", {
    class: "badge",
    hidden: true,
    title: T.home.sourceHealthTooltip,
  });
  const mainsFreqBanner = buildMainsFrequencyBanner();
  const cacsiBanner = buildCacsiRegulationBanner();
  let currentSource = "";
  let latestConfig: Awaited<ReturnType<typeof api.getConfig>>["config"] | undefined;
  const sourceCta = h("p", { class: "banner banner--warn", role: "status", hidden: true });
  const pmqttBindingsBanner = h("p", {
    class: "banner banner--warn",
    role: "alert",
    hidden: true,
  });
  const vacationBanner = h("p", { class: "banner banner--info", role: "status", hidden: true });
  const safetyLockoutBanner = h("p", {
    class: "banner banner--danger",
    role: "alert",
    hidden: true,
  });
  const operationalBanner = h("p", { class: "banner banner--info", role: "status", hidden: true });
  const heaterBackoffBanner = h("p", { class: "banner banner--info", role: "status", hidden: true });
  let safetyLocked = false;

  function updateOperationalBanner(st: Awaited<ReturnType<typeof api.getState>>) {
    const s = st.status;
    if (!s?.device_lifecycle) {
      operationalBanner.hidden = true;
      return;
    }
    const lifecycle =
      T.home.lifecycleWire[s.device_lifecycle as keyof typeof T.home.lifecycleWire] ??
      s.device_lifecycle;
    const lines: string[] = [T.home.operationalStatus.replace("{lifecycle}", lifecycle)];
    if (s.output_suspend?.active) {
      const reason =
        T.home.suspendReasonWire[
          s.output_suspend.reason as keyof typeof T.home.suspendReasonWire
        ] ?? s.output_suspend.reason;
      lines.push(T.home.operationalSuspended.replace("{reason}", reason));
    } else if (hasSurplusRegulation() && s.regulation_motion && s.regulation_motion !== "idle") {
      const motion =
        T.home.regulationMotionWire[
          s.regulation_motion as keyof typeof T.home.regulationMotionWire
        ] ?? s.regulation_motion;
      lines.push(T.home.operationalMotion.replace("{motion}", motion));
    }
    operationalBanner.textContent = lines.join(" — ");
    operationalBanner.hidden =
      s.device_lifecycle === "regulating" &&
      !s.output_suspend?.active &&
      (!s.regulation_motion || s.regulation_motion === "idle");
  }
  const dateEl = h("p", { class: "card__sub", "aria-live": "polite" }, "");
  const probeTempEl = h("p", { class: "card__sub", hidden: true, "aria-live": "polite" });
  const legionellaBanner = buildLegionellaGatingBanner();
  let triacGatingDisabled = false;

  const dashboardCard = h(
    "section",
    { class: "card" },
    h(
      "header",
      { class: "card__head" },
      h(
        "div",
        {},
        h("h2", { class: "card__title" }, T.nav.home),
        dateEl,
        probeTempEl,
      ),
      tariff.el,
      sourceHealthBadge,
    ),
    h(
      "div",
      { class: "kpi-grid kpi-grid--instant" },
      kImport.el,
      kExport.el,
      kApparentImp.el,
    ),
    h("div", { class: "kpi-grid kpi-grid--daily" }, kEnergyImp.el, kEnergyExp.el, kTemp.el),
  );
  kTemp.el.hidden = true;

  const sk1 = buildKpi({ label: T.home.chartSeriesActive, unit: "W", variant: "import" });
  const sk2 = buildKpi({ label: T.home.chartSeriesApparent, unit: "VA", variant: "apparent" });
  const sk3 = buildKpi({ label: T.home.secondaryEnergyDay, variant: "energy" });
  const secondaryTitle = h("h2", { class: "card__title" }, T.home.secondaryHeader);
  const secondaryCard = h(
    "section",
    { class: "card", hidden: true },
    h("header", { class: "card__head" }, h("div", {}, secondaryTitle)),
    h("div", { class: "kpi-grid" }, sk1.el, sk2.el, sk3.el),
  );

  const chartNoData = h("p", {
    class: "banner banner--warn",
    role: "status",
    hidden: true,
  }, T.home.chartNoData);

  const chart = buildChart({
    title: T.home.chartTitle,
    yLabel: "W",
    y2Label: "VA",
    height: 220,
    series: powerChartSeriesSpecs({
      houseActive: `${T.home.chartSeriesActive} (${T.home.netLabel})`,
      triacActive: `${T.home.chartSeriesActive} (${T.history.legendTriac})`,
      houseApparent: `${T.home.chartSeriesApparent} (${T.home.netLabel})`,
      triacApparent: `${T.home.chartSeriesApparent} (${T.history.legendTriac})`,
    }),
    data: [[0], [0], [0], [0], [0]] as unknown as [
      number[],
      number[],
      number[],
      number[],
      number[],
    ],
    xFormat: (v) => formatPowerAxisSeconds(v),
  });
  const tempChart = buildChart({
    title: T.home.chart10mTempTitle,
    yLabel: "°C",
    height: 160,
    intLegend: false,
    series: [{ label: T.home.temperature, color: "var(--c-temp)", unit: "°C" }],
    data: [[0], [0]] as unknown as [number[], number[]],
    xFormat: (v) => formatPowerAxisSeconds(v),
  });
  const tempChartCard = h("section", { class: "card chart-card", hidden: true }, tempChart.el);

  const chartCard = h("section", { class: "card chart-card" }, chartNoData, chart.el);

  function syncChartLabels() {
    const d = deviceInfo.get();
    const house = d?.probe_house_name || T.history.legendHouse;
    const triac = d?.probe_second_name || T.history.legendTriac;
    chart.setSeriesLabels([
      `${T.home.chartSeriesActive} ${house}`,
      `${T.home.chartSeriesActive} ${triac}`,
      `${T.home.chartSeriesApparent} ${house}`,
      `${T.home.chartSeriesApparent} ${triac}`,
    ]);
  }
  syncChartLabels();

  function temperatureLabel(): string {
    return deviceInfo.get()?.temperature_label || T.home.temperature;
  }

  function applyHomeHistory(j: HistoryPower) {
    const house = j.house_active_w || [];
    const triac = j.triac_active_w || [];
    const vaH = j.house_apparent_va || [];
    const vaT = j.triac_apparent_va || [];
    const n = Math.max(house.length, triac.length, vaH.length, vaT.length, 1);
    const period = j.sample_period_s > 0 ? j.sample_period_s : 2;
    const xs = buildPowerTimeAxisSeconds(n, period);
    chart.setData([
      xs,
      padSeries(house, n),
      padSeries(triac, n),
      padSeries(vaH, n),
      padSeries(vaT, n),
    ] as unknown as [number[], number[], number[], number[], number[]]);
    chartNoData.hidden = powerHasSignal(house, triac);

    const tempSeries = j.temperature_series_c || [];
    const showTempChart =
      historySeriesHasProbeTemp(tempSeries) ||
      isProbeTemperatureReading(j.temperature_now_c);
    tempChartCard.hidden = !showTempChart;
    if (showTempChart && historySeriesHasProbeTemp(tempSeries)) {
      const tn = tempSeries.length;
      const txs = buildPowerTimeAxisSeconds(tn, period);
      tempChart.setSeriesLabels([temperatureLabel()]);
      tempChart.setData([txs, tempSeries] as unknown as [number[], number[]]);
    }
  }

  function updateLiveTemperature(
    liveTempC: number | undefined,
    sensorAvailable: boolean,
  ) {
    if (isProbeTemperatureReading(liveTempC)) {
      const lbl = temperatureLabel();
      probeTempEl.hidden = false;
      probeTempEl.textContent = `${lbl}: ${fmtTempC(liveTempC)}`;
      kTemp.el.hidden = false;
      kTemp.setValue(fmtTempC(liveTempC));
      kTemp.setStale(false);
    } else {
      probeTempEl.hidden = true;
      kTemp.el.hidden = true;
    }
    if (!tempChartCard.hidden && isProbeTemperatureReading(liveTempC)) {
      kTemp.el.hidden = false;
    }
    if (!sensorAvailable && !isProbeTemperatureReading(liveTempC)) {
      tempChartCard.hidden = true;
    }
  }

  async function loadHomeHistory() {
    try {
      const j = await api.getHistoryPower("10m", 120, { signal });
      applyHomeHistory(j);
    } catch {
      chartNoData.hidden = false;
    }
  }

  const actionsTitle = h("h2", { class: "card__title" }, T.home.activeActions);
  const actionsBody = h(
    "div",
    { class: "table-wrap", hidden: false },
    h(
      "table",
      { class: "table" },
      h(
        "thead",
        {},
        h(
          "tr",
          {},
          h("th", {}, T.actions.actionTitle),
          h("th", { class: "num" }, T.home.overrideColumn),
          h("th", { class: "num" }, T.on),
        ),
      ),
      h("tbody", { id: "active-rows" }),
    ),
  );
  const actionsEmpty = h(
    "p",
    { class: "empty", hidden: true },
    T.home.noActiveActions,
  );
  const tempLine = h("p", { class: "card__sub", hidden: true });
  const actionsCard = h(
    "section",
    { class: "card" },
    h("header", { class: "card__head" }, actionsTitle),
    actionsBody,
    actionsEmpty,
    tempLine,
  );

  const syncSurplusUi = () => {
    actionsCard.hidden = !hasSurplusRegulation();
  };
  syncSurplusUi();

  function syncPmqttBindingsBanner(config: typeof latestConfig) {
    const show = config ? pmqttBindingsMissing(config) : false;
    pmqttBindingsBanner.hidden = !show;
    if (!show) return;
    pmqttBindingsBanner.replaceChildren(
      T.home.pmqttBindingsMissing + " ",
      h(
        "button",
        {
          type: "button",
          class: "btn btn--ghost btn--sm",
          onClick: () => {
            if (!latestConfig) return;
            openSourceSetupWizard({
              initialConfig: latestConfig,
              signal,
              mode: "edit_pmqtt",
              lockSource: "Pmqtt",
              onSaved: (c) => {
                latestConfig = c;
                currentSource = c.source || "";
                syncPmqttBindingsBanner(c);
              },
            });
          },
        },
        T.home.pmqttBindingsSetup,
      ),
    );
  }

  function updateSafetyLockoutBanner(st: Awaited<ReturnType<typeof api.getState>>) {
    safetyLocked = isSafetyLockoutActive(null, st.status ?? null);
    safetyLockoutBanner.hidden = !safetyLocked;
    if (safetyLocked) {
      safetyLockoutBanner.replaceChildren(
        T.home.safetyLockoutBanner + " ",
        h(
          "a",
          {
            href: withBase("/diag"),
            class: "link",
            "data-route": "true",
          },
          T.nav.diag,
        ),
      );
    }
  }

  outlet.append(
    mainsFreqBanner.el,
    cacsiBanner.el,
    vacationBanner,
    safetyLockoutBanner,
    operationalBanner,
    heaterBackoffBanner,
    sourceCta,
    pmqttBindingsBanner,
    legionellaBanner.el,
    dashboardCard,
    secondaryCard,
    chartCard,
    tempChartCard,
    actionsCard,
  );

  void api.getActionsConfig({ signal }).then((ac) => {
    const triac = ac.actions.find((a) => a.index === 0) ?? ac.actions[0];
    triacGatingDisabled = !!(triac && !triacPeriodsHaveTemperatureGating(triac.periods));
    legionellaBanner.update({ gatingDisabled: triacGatingDisabled, temperatureC: -127 });
  }).catch(() => {});

  let overrides = new Map<number, ActionOverride>();

  async function applyOverride(index: number, state: OverrideState) {
    try {
      if (state === "auto") {
        await api.clearActionOverride(index, { signal });
      } else {
        await api.postActionOverride(index, { state }, { signal });
      }
    } catch (e) {
      if (e instanceof ApiError && e.status === 400) {
        const msg =
          typeof e.body === "object" &&
          e.body &&
          "error" in e.body &&
          typeof (e.body as { error: unknown }).error === "string"
            ? (e.body as { error: string }).error
            : "";
        if (msg.toLowerCase().includes("temperature cap")) {
          toast(T.actions.overrideTempCapBlocked, "error");
          return;
        }
      }
      toast(T.saveError, "error");
    }
  }

  async function applyTriacFullOverride(index: number) {
    if (!(await confirmLegionellaTriacFull())) return;
    try {
      await api.postActionOverride(
        index,
        { state: "triac_fixed", triac_open_percent: 100 },
        { signal },
      );
    } catch (e) {
      if (e instanceof ApiError && e.status === 400) {
        toast(T.actions.overrideTempCapBlocked, "error");
        return;
      }
      toast(T.saveError, "error");
    }
  }

  return routeCleanup(signal, (scope) => {
    const unsubDevice = deviceInfo.subscribe((d) => {
      if (d?.probe_second_name) secondaryTitle.textContent = d.probe_second_name;
      const lbl = d?.temperature_label || T.home.temperature;
      kTemp.el.querySelector(".kpi__label")!.textContent = lbl;
      syncChartLabels();
      syncSurplusUi();
    });
    scope.onUnmount(() => {
      unsubDevice();
      chart.destroy();
      tempChart.destroy();
    });

    void loadHomeHistory();
    scope.trackPoll(
      poll(
        async () => {
          await loadHomeHistory();
        },
        6000,
        3000,
        { immediate: false },
      ),
    );

    let tick = 0;
    const tbody = actionsBody.querySelector("tbody")!;

    scope.trackPoll(
      poll(
        async () => {
          tick += 1;
          let liveTempC: number | undefined;
          let sensorAvailable = false;
          const m = await api.getMeasurements({ signal }).catch(() => null);
          if (m) {
            liveTempC = resolveLiveTemperatureC(m);
            sensorAvailable = temperatureSensorAvailable(m);
            dateEl.textContent = m.date;
          } else {
            const live = await fetchLiveTemperatureTelemetry({ signal });
            liveTempC = resolveLiveTemperatureC(live);
            sensorAvailable = temperatureSensorAvailable(live);
          }
          updateLiveTemperature(liveTempC, sensorAvailable);
          if (!m) {
            if (tick % 2 === 0) {
              const st = await api.getState({ signal }).catch(() => null);
              if (st) {
                const stTemp = resolveLiveTemperatureC(st);
                updateLiveTemperature(
                  stTemp ?? liveTempC,
                  sensorAvailable || temperatureSensorAvailable(st),
                );
              }
            }
            return;
          }
          const tariffText = m.linky_tariff || m.ltarf;
          if (tariffText) {
            tariff.setTariff(tariffText);
            tariff.el.hidden = false;
          } else {
            tariff.setTariff(null);
            tariff.el.hidden = true;
          }
          kImport.setValue(fmtPowerW(m.house.active_import_w));
          kExport.setValue(fmtPowerW(m.house.active_export_w));
          kApparentImp.setValue(fmtPowerW(m.house.apparent_import_va));
          const ej = fmtEnergyWh(m.house.energy_day_import_wh);
          kEnergyImp.setValue(ej.value, ej.unit);
          const eji = fmtEnergyWh(m.house.energy_day_export_wh);
          kEnergyExp.setValue(eji.value, eji.unit);
          [kImport, kExport, kApparentImp, kEnergyImp, kEnergyExp].forEach((k) =>
            k.setStale(false),
          );
          const secondLive =
            !!m.second &&
            (m.second.energy_total_import_wh > 0 ||
              m.second.energy_total_export_wh > 0 ||
              m.second.active_import_w !== 0 ||
              m.second.active_export_w !== 0 ||
              (m.raw_meter?.voltage_second_v ?? 0) > 0 ||
              (m.raw_meter?.current_second_a ?? 0) > 0);
          if (hasTriac() && secondLive) {
            secondaryCard.hidden = false;
            sk1.setValue(
              fmtPowerW(m.second.active_import_w - m.second.active_export_w),
            );
            sk2.setValue(fmtPowerW(m.second.apparent_import_va));
            const e2 = fmtEnergyWh(m.second.energy_day_export_wh);
            sk3.setValue(e2.value, e2.unit);
          } else {
            secondaryCard.hidden = true;
          }

          if (tick % 2 === 0) {
            const st = await api.getState({ signal });
            const tempC = resolveLiveTemperatureC(st) ?? -127;
            if (!isProbeTemperatureReading(liveTempC) && isProbeTemperatureReading(tempC)) {
              updateLiveTemperature(tempC, sensorAvailable || temperatureSensorAvailable(st));
            }
            if (hasTriac()) {
              legionellaBanner.update({
                gatingDisabled: triacGatingDisabled,
                temperatureC: tempC,
              });
            } else {
              legionellaBanner.el.hidden = true;
            }
            heaterBackoffBanner.hidden = !hasSurplusRegulation() || !st.heater_load_backoff_active;
            if (st.heater_load_backoff_active && hasSurplusRegulation()) {
              heaterBackoffBanner.textContent = T.home.heaterLoadBackoffActive;
            }
            updateOperationalBanner(st);
            updateSafetyLockoutBanner(st);
            overrides = overrideMap(st.override_summary);
            const j = st.actions_live;
            const slots = j.active_slots ?? [];
            tbody.replaceChildren();
            if (!slots.length) {
              actionsEmpty.hidden = false;
              actionsBody.hidden = true;
            } else {
              actionsEmpty.hidden = true;
              actionsBody.hidden = false;
              for (const s of slots) {
                const valueEl =
                  s.index === 0 && typeof s.triac_open_percent === "number"
                    ? buildTriacGauge(s.triac_open_percent)
                    : h(
                        "span",
                        {
                          class: "tariff",
                          style: `--tariff-color: var(${s.on ? "--c-ok" : "--c-fg-faint"})`,
                        },
                        h("span", { class: "tariff__chip" }),
                        s.on ? T.on : T.off,
                      );
                tbody.append(
                  h(
                    "tr",
                    {},
                    h("td", {}, s.title || "—"),
                    h(
                      "td",
                      { class: "num" },
                      buildOverrideControls({
                        index: s.index,
                        active: overrides.get(s.index),
                        disabled: safetyLocked,
                        onSet: (state) => applyOverride(s.index, state),
                        onTriacFull:
                          s.index === 0
                            ? () => applyTriacFullOverride(s.index)
                            : undefined,
                      }),
                    ),
                    h("td", { class: "num" }, valueEl),
                  ),
                );
              }
            }
            const actionsTemp = resolveLiveTemperatureC(j);
            if (isProbeTemperatureReading(actionsTemp)) {
              tempLine.hidden = false;
              tempLine.textContent = `${temperatureLabel()}: ${fmtTempC(actionsTemp)}`;
            } else {
              tempLine.hidden = true;
            }
          }

          if (tick % 5 === 0) {
            try {
              const res = await api.getConfig({ signal });
              const config = res.config;
              latestConfig = config;
              currentSource = config.source || "";
              tariff.el.hidden = false;
              mainsFreqBanner.setWarning(config.mains_frequency_warning, {
                effectiveHz: config.mains_frequency_effective_hz,
                source: config.mains_frequency_source,
                installCountry: config.install_country,
              });
              vacationBanner.hidden = !config.vacation_enabled;
              if (config.vacation_enabled) {
                vacationBanner.textContent = T.home.vacationActive;
              }
            } catch {
              // Config may require login when HTTP API auth is on.
            }
            sourceCta.hidden = currentSource !== "NotDef";
            if (!sourceCta.hidden) {
              sourceCta.replaceChildren(
                T.home.sourceNotConfigured + " ",
                h(
                  "button",
                  {
                    type: "button",
                    class: "btn btn--ghost btn--sm",
                    onClick: () => {
                      if (!latestConfig) return;
                      openSourceSetupWizard({
                        initialConfig: latestConfig,
                        signal,
                        onSaved: (c) => {
                          currentSource = c.source;
                          latestConfig = c;
                          sourceCta.hidden = true;
                          syncPmqttBindingsBanner(c);
                        },
                      });
                    },
                  },
                  T.home.setupSource,
                ),
              );
            }
            syncPmqttBindingsBanner(latestConfig);
          }

          if (tick % 10 === 0 && currentSource === "Linky") {
            const d = await api.getSourceDiagnostics(128, { signal });
            cacsiBanner.setVisible(!!d.linky?.cacsi_no_export);
            const score = d.diagnostics?.health_score;
            if (score != null) {
              sourceHealthBadge.hidden = false;
              sourceHealthBadge.textContent = `${T.home.sourceHealthScore}: ${score}`;
            } else {
              sourceHealthBadge.hidden = true;
            }
          } else if (currentSource !== "Linky") {
            cacsiBanner.setVisible(false);
          }
        },
        3000,
        5000,
        { startDelayMs: 200 },
      ),
    );

  });
}

function buildTriacGauge(percent: number): HTMLElement {
  const pct = Math.max(0, Math.min(100, Math.round(percent)));
  const wrap = h("span", {
    class: "row",
    style: "gap:8px;justify-content:flex-end;align-items:center;",
  });
  const bar = h("span", {
    style:
      "position:relative;display:inline-block;width:120px;height:8px;border-radius:999px;background:var(--c-bg-sunk);overflow:hidden;",
  });
  bar.append(
    h("span", {
      style: `position:absolute;inset:0;width:${pct}%;background:var(--c-warn);`,
    }),
  );
  wrap.append(bar, h("span", {}, fmtPercent(pct)));
  return wrap;
}
