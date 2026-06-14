import { h } from "../../utils/dom";

export interface PageHeaderOptions {
  title: string;
  description?: string;
  actions?: HTMLElement[];
  className?: string;
}

export function buildPageHeader(opts: PageHeaderOptions): HTMLElement {
  const header = h("header", {
    class: `page-header${opts.className ? ` ${opts.className}` : ""}`,
  });
  const main = h("div", { class: "page-header__main" }, h("h1", { class: "page-title" }, opts.title));
  if (opts.description?.trim()) {
    main.append(h("p", { class: "page-header__desc" }, opts.description));
  }
  header.append(main);
  if (opts.actions?.length) {
    header.append(h("div", { class: "page-header__actions" }, ...opts.actions));
  }
  return header;
}
