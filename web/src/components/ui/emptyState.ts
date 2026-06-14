import { h } from "../../utils/dom";

export interface EmptyStateOptions {
  message: string;
  action?: HTMLElement;
}

export function buildEmptyState(opts: EmptyStateOptions): HTMLElement {
  const el = h("div", { class: "empty" }, h("p", {}, opts.message));
  if (opts.action) el.append(opts.action);
  return el;
}
