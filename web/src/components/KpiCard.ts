import { h } from "../utils/dom";

export type KpiVariant = "import" | "export" | "apparent" | "energy" | "temp";

export interface KpiCardOptions {
  label: string;
  variant?: KpiVariant;
  unit?: string;
  sub?: string;
}

export interface KpiCard {
  el: HTMLElement;
  setValue(value: string, sub?: string): void;
  setStale(stale: boolean): void;
}

export function buildKpi(opts: KpiCardOptions): KpiCard {
  const valueEl = h("span", { class: "kpi__value" }, "—");
  const unitEl = opts.unit ? h("span", { class: "kpi__unit" }, opts.unit) : null;
  const subEl = h("span", { class: "kpi__sub" }, opts.sub ?? "");
  const valueWrap = h(
    "span",
    {
      "aria-live": "polite",
      "aria-atomic": "true",
    },
    valueEl,
    unitEl,
  );
  const card = h(
    "article",
    {
      class: `kpi${opts.variant ? " kpi--" + opts.variant : ""}`,
      "data-stale": "false",
    },
    h("span", { class: "kpi__label" }, opts.label),
    valueWrap,
    subEl,
  );
  return {
    el: card,
    setValue(value: string, sub?: string) {
      valueEl.textContent = value;
      if (sub !== undefined) subEl.textContent = sub;
    },
    setStale(stale: boolean) {
      card.dataset.stale = stale ? "true" : "false";
    },
  };
}
