import { api } from "../api/client";
import type { RouterConfig, SourceDiagnostics } from "../api/types";
import { getStrings } from "../i18n";
import { poll, type PollHandle } from "../state/store";
import { h } from "../utils/dom";
import {
  buildSourceSummaryRows,
  type SourceSummaryRow,
} from "../routes/settings/measurementSourceSummary";
import {
  pinoutSectionUrl,
  registryEntry,
  type SourceWireId,
} from "../routes/settings/sourceRegistry";
import { pmqttBindingsMissing } from "../utils/pmqttBindings";

export interface MeasurementSourceSummaryHandle {
  el: HTMLElement;
  setConfig(cfg: RouterConfig): void;
  stop(): void;
}

export function buildMeasurementSourceSummary(opts: {
  signal: AbortSignal;
  initialConfig: RouterConfig;
}): MeasurementSourceSummaryHandle {
  const T = getStrings();
  let cfg = opts.initialConfig;
  let diag: SourceDiagnostics | null = null;
  let loadState: "loading" | "ok" | "err" = "loading";
  let pollHandle: PollHandle | undefined;

  const wrap = h("div", { class: "source-summary" });
  const pmqttWarnEl = h("p", {
    class: "banner banner--warn",
    role: "alert",
    hidden: true,
  });
  const statusEl = h("p", { class: "field__hint source-summary__loading" }, T.loading);
  const tableWrap = h("div", { class: "table-wrap", hidden: true });
  const table = h("table", { class: "table source-summary__table" }, h("tbody", {}));
  const pinoutEl = h("p", { class: "field__hint", hidden: true });
  tableWrap.append(table);
  wrap.append(pmqttWarnEl, statusEl, tableWrap, pinoutEl);

  function syncPmqttWarn() {
    pmqttWarnEl.hidden = !pmqttBindingsMissing(cfg);
    if (!pmqttWarnEl.hidden) {
      pmqttWarnEl.textContent = T.home.pmqttBindingsMissing;
    }
  }

  function renderRows(rows: SourceSummaryRow[]) {
    const tbody = table.querySelector("tbody")!;
    tbody.replaceChildren(
      ...rows.map((row) => {
        const valueEl = h("td", { class: "num" }, row.value);
        if (row.status === "ok") {
          valueEl.classList.add("source-summary__status--ok");
        } else if (row.status === "warn") {
          valueEl.classList.add("source-summary__status--warn");
        } else if (row.status === "err") {
          valueEl.classList.add("source-summary__status--err");
        }
        return h("tr", {}, h("td", {}, row.label), valueEl);
      }),
    );
  }

  function renderPinout() {
    const wire = (cfg.source || "JsyMk194") as SourceWireId;
    const entry = registryEntry(wire);
    if (!entry) {
      pinoutEl.hidden = true;
      return;
    }
    pinoutEl.hidden = false;
    pinoutEl.replaceChildren(
      h(
        "a",
        {
          href: pinoutSectionUrl(entry.pinoutAnchor),
          target: "_blank",
          rel: "noopener",
        },
        T.settings.sourceSummary.openPinout,
      ),
    );
  }

  function render() {
    syncPmqttWarn();
    renderPinout();
    if (loadState === "loading" && !diag) {
      statusEl.hidden = false;
      statusEl.textContent = T.loading;
      tableWrap.hidden = true;
      return;
    }
    if (loadState === "err" && !diag) {
      statusEl.hidden = false;
      statusEl.textContent = T.settings.sourceSummary.loadError;
      tableWrap.hidden = true;
      return;
    }
    statusEl.hidden = true;
    tableWrap.hidden = false;
    renderRows(buildSourceSummaryRows(cfg, diag, T));
  }

  async function fetchDiag() {
    if (opts.signal.aborted) return;
    diag = await api.getSourceDiagnostics(128, { signal: opts.signal });
    loadState = "ok";
    render();
  }

  render();
  pollHandle = poll(
    async () => {
      try {
        await fetchDiag();
        loadState = "ok";
      } catch (e) {
        if ((e as DOMException)?.name === "AbortError") return;
        if (!diag) loadState = "err";
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
