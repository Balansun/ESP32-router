import { h } from "../../../utils/dom";

/** Standard settings card wrapper. */
export function settingsSection(title: string, ...rows: HTMLElement[]): HTMLElement {
  return h(
    "section",
    { class: "card" },
    h("h2", { class: "section__title" }, title),
    ...rows,
  );
}
