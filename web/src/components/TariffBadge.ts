import { h } from "../utils/dom";
import { resolveTariffDisplay } from "../utils/tariffDisplay";

export interface TariffBadge {
  el: HTMLElement;
  setTariff(text: string | null | undefined): void;
}

export function buildTariffBadge(prefixLabel: string): TariffBadge {
  const chip = h("span", { class: "tariff__chip" });
  const txt = h("span", {}, "—");
  const badge = h(
    "span",
    {
      class: "tariff",
      "aria-label": prefixLabel,
      hidden: true,
    },
    chip,
    txt,
  );
  function setTariff(text: string | null | undefined) {
    if (!text) {
      badge.hidden = true;
      return;
    }
    const matched = resolveTariffDisplay(text);
    if (!matched) {
      badge.hidden = true;
      return;
    }
    txt.textContent = matched.label;
    chip.style.setProperty("--tariff-color", matched.color);
    badge.hidden = false;
  }
  return { el: badge, setTariff };
}
