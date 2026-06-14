import { h } from "../../utils/dom";

export function buildSkeleton(lines = 1, block = false): HTMLElement {
  const wrap = h("div", { class: "stack stack--sm", "aria-hidden": "true" });
  for (let i = 0; i < lines; i++) {
    wrap.append(h("div", { class: block && i === 0 ? "skeleton skeleton--block" : "skeleton" }));
  }
  return wrap;
}
