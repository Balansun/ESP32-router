import { h } from "../utils/dom";

export function kvRow(label: string, value: string): HTMLElement {
  return h(
    "div",
    { class: "field" },
    h("span", { class: "field__label" }, label),
    h("p", { class: "card__sub", style: "margin:0;word-break:break-all;" }, value),
  );
}
