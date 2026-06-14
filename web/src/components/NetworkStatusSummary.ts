import { api } from "../api/client";
import type { RouterConfig, SystemInfo, WifiInfo } from "../api/types";
import { getStrings } from "../i18n";
import { poll, type PollHandle } from "../state/store";
import { h } from "../utils/dom";
import {
  applyLiveNetworkPrefill,
  isUnsetIp,
  type LiveNetworkFields,
} from "../utils/networkIp";

export interface NetworkStatusSummaryHandle {
  el: HTMLElement;
  setConfig(cfg: RouterConfig): void;
  stop(): void;
}

function fmtIp(v: string | undefined): string {
  if (isUnsetIp(v)) return "—";
  return (v ?? "").trim();
}

function fmtRssi(db: number): string {
  if (!Number.isFinite(db) || db === 0) return "—";
  return `${db} dBm`;
}

export function buildNetworkStatusSummary(opts: {
  signal: AbortSignal;
  initialConfig: RouterConfig;
  /** When Wi‑Fi is connected, copy live IPs into inputs that are still 0.0.0.0 / empty. */
  networkFields?: LiveNetworkFields;
}): NetworkStatusSummaryHandle {
  const T = getStrings();
  let cfg = opts.initialConfig;
  let wifi: WifiInfo | null = null;
  let system: SystemInfo | null = null;
  let loadState: "loading" | "ok" | "err" = "loading";
  let pollHandle: PollHandle | undefined;

  const wrap = h("div", { class: "network-live" });
  const statusEl = h("p", { class: "field__hint network-live__loading" }, T.loading);
  const tableWrap = h("div", { class: "table-wrap", hidden: true });
  const table = h("table", { class: "table network-live__table" }, h("tbody", {}));
  tableWrap.append(table);
  wrap.append(statusEl, tableWrap);

  function row(label: string, value: string) {
    return h("tr", {}, h("td", {}, label), h("td", { class: "num" }, value));
  }

  function renderRows() {
    const tbody = table.querySelector("tbody")!;
    if (!wifi) {
      tbody.replaceChildren();
      return;
    }

    const modeValue = wifi.mode === "ap" ? T.wifi.modeAp : T.wifi.modeSta;
    const connValue = wifi.connected ? T.wifi.connectedYes : T.wifi.connectedNo;
    const ipAssignValue = cfg.dhcp_on ? T.settings.dhcp : T.settings.networkLive.configured;

    if (!wifi.connected) {
      tbody.replaceChildren(
        row(T.settings.networkLive.mode, modeValue),
        row(T.settings.networkLive.connection, connValue),
        row(T.diag.ip, fmtIp(wifi.ip || system?.ip)),
      );
      return;
    }

    const ssid = (wifi.ssid || system?.ssid || "").trim() || "—";
    tbody.replaceChildren(
      row(T.settings.networkLive.mode, modeValue),
      row(T.settings.networkLive.connection, connValue),
      row(T.settings.networkLive.ipAssignment, ipAssignValue),
      row(T.diag.ssid, ssid),
      row(T.diag.ip, fmtIp(system?.ip || wifi.ip)),
      row(T.diag.gateway, fmtIp(system?.gateway)),
      row(T.diag.subnet, fmtIp(system?.subnet)),
      row(T.diag.dns, fmtIp(system?.dns)),
      row(T.diag.mac, system?.mac?.trim() || "—"),
      row(T.diag.bssid, system?.wifi_bssid?.trim() || "—"),
      row(T.diag.rssi, fmtRssi(system?.wifi_rssi_dbm ?? wifi.rssi)),
    );
  }

  function render() {
    if (loadState === "loading" && !wifi) {
      statusEl.hidden = false;
      statusEl.textContent = T.loading;
      tableWrap.hidden = true;
      return;
    }
    if (loadState === "err" && !wifi) {
      statusEl.hidden = false;
      statusEl.textContent = T.settings.sourceSummary.loadError;
      tableWrap.hidden = true;
      return;
    }
    if (!wifi?.connected) {
      statusEl.hidden = false;
      statusEl.textContent = T.settings.networkLive.notConnected;
      tableWrap.hidden = false;
      renderRows();
      return;
    }
    statusEl.hidden = true;
    tableWrap.hidden = false;
    renderRows();
  }

  async function fetchLive() {
    if (opts.signal.aborted) return;
    const [w, s] = await Promise.all([
      api.getWifi({ signal: opts.signal }),
      api.getSystem({ signal: opts.signal }),
    ]);
    wifi = w;
    system = s;
    loadState = "ok";
    if (w.connected && s && opts.networkFields) {
      applyLiveNetworkPrefill(opts.networkFields, s);
    }
    render();
  }

  render();
  pollHandle = poll(
    async () => {
      try {
        await fetchLive();
        loadState = "ok";
      } catch (e) {
        if ((e as DOMException)?.name === "AbortError") return;
        if (!wifi) loadState = "err";
        render();
        throw e;
      }
    },
    5000,
    10000,
  );

  opts.signal.addEventListener(
    "abort",
    () => {
      pollHandle?.stop();
    },
    { once: true },
  );

  return {
    el: wrap,
    setConfig(next) {
      cfg = next;
      render();
    },
    stop() {
      pollHandle?.stop();
    },
  };
}
