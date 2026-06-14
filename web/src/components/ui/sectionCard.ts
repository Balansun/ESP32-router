import { h } from "../../utils/dom";

export interface SectionCardOptions {
  title: string;
  description?: string;
  children: HTMLElement[];
  className?: string;
}

export function buildSectionCard(opts: SectionCardOptions): HTMLElement {
  const card = h("section", {
    class: `card${opts.className ? ` ${opts.className}` : ""}`,
  });
  card.append(h("h2", { class: "section__title" }, opts.title));
  if (opts.description?.trim()) {
    card.append(h("p", { class: "card__desc" }, opts.description));
  }
  card.append(...opts.children);
  return card;
}
