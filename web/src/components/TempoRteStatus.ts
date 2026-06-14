import { api } from "../api/client";
import type { TempoTariffStatus } from "../api/types";
import { getStrings } from "../i18n";
import { h } from "../utils/dom";
import {
  mergeTempoTariffStatus,
  resolveTariffDisplay,
  resolveTempoTariffDisplay,
} from "../utils/tariffDisplay";

export interface TempoRteStatus {
  el: HTMLElement;
  setEnabled(enabled: boolean): void;
  refresh(): void;
  stop(): void;
}

export function buildTempoRteStatus(signal: AbortSignal): TempoRteStatus {
  const T = getStrings();
  const chip = h("span", { class: "tariff__chip" });
  const labelEl = h("span", {}, "");
  const badge = h(
    "span",
    { class: "tariff tempo-rte-status__badge", hidden: true },
    chip,
    labelEl,
  );
  const pendingEl = h(
    "p",
    { class: "field__hint tempo-rte-status__pending", hidden: true },
    T.settings.tempoRteStatusPending,
  );
  const staleEl = h(
    "p",
    { class: "field__hint tempo-rte-status__stale", hidden: true },
    T.settings.tempoRteStatusStale,
  );
  const fetchFailedEl = h(
    "p",
    { class: "field__hint tempo-rte-status__fetch-failed", hidden: true },
    T.settings.tempoRteStatusFetchFailed,
  );
  const el = h(
    "div",
    { class: "tempo-rte-status", hidden: true, "aria-live": "polite" },
    h("p", { class: "field__hint tempo-rte-status__label" }, T.settings.tempoRteStatusToday),
    badge,
    pendingEl,
    staleEl,
    fetchFailedEl,
  );

  let enabled = false;
  let timer: number | undefined;

  function applyStatus(status: TempoTariffStatus) {
    const display = resolveTempoTariffDisplay(status);
    const matched = display ? resolveTariffDisplay(display.text) : null;
    if (matched) {
      labelEl.textContent = display!.isTomorrow
        ? `${T.settings.tempoRteStatusTomorrow}: ${matched.label}`
        : matched.label;
      chip.style.setProperty("--tariff-color", matched.color);
      badge.hidden = false;
      pendingEl.hidden = true;
      fetchFailedEl.hidden = true;
    } else {
      badge.hidden = true;
      const noFetchYet = status.enabled && (status.last_fetch_epoch ?? 0) === 0;
      if (noFetchYet) {
        pendingEl.hidden = true;
        fetchFailedEl.textContent = T.settings.tempoRteStatusFetchFailed;
        fetchFailedEl.hidden = false;
      } else {
        pendingEl.hidden = false;
        fetchFailedEl.hidden = true;
      }
    }
    staleEl.hidden = !status.stale;
  }

  function clearDisplay() {
    badge.hidden = true;
    pendingEl.hidden = true;
    staleEl.hidden = true;
    fetchFailedEl.hidden = true;
  }

  async function fetchStatus(): Promise<TempoTariffStatus | null> {
    const measurements = await api.getMeasurements({ signal }).catch(() => null);
    try {
      const tempo = await api.getTariffTempo({ signal });
      return mergeTempoTariffStatus(tempo, measurements);
    } catch {
      if (!measurements) return null;
      return mergeTempoTariffStatus(
        { enabled: true, stale: false },
        measurements,
      );
    }
  }

  async function refresh() {
    if (!enabled || signal.aborted) return;
    try {
      const status = await fetchStatus();
      if (signal.aborted || !enabled) return;
      if (status) applyStatus(status);
      else if (enabled) {
        badge.hidden = true;
        pendingEl.hidden = false;
        staleEl.hidden = true;
      }
    } catch {
      if (!signal.aborted && enabled) {
        badge.hidden = true;
        pendingEl.hidden = false;
        staleEl.hidden = true;
      }
    }
  }

  function setEnabled(on: boolean) {
    enabled = on;
    el.hidden = !on;
    if (!on) {
      if (timer) {
        clearInterval(timer);
        timer = undefined;
      }
      clearDisplay();
      return;
    }
    void refresh();
    if (!timer) {
      timer = window.setInterval(() => void refresh(), 30_000);
    }
  }

  function stop() {
    enabled = false;
    if (timer) {
      clearInterval(timer);
      timer = undefined;
    }
  }

  signal.addEventListener("abort", stop, { once: true });

  return { el, setEnabled, refresh, stop };
}
