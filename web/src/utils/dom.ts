// Minimal DOM helpers — keeps each route file readable without a framework.

type Attrs = Record<string, string | number | boolean | null | undefined |
  ((e: Event) => void)>;

export function h<K extends keyof HTMLElementTagNameMap>(
  tag: K,
  attrs: Attrs = {},
  ...children: Array<Node | string | null | undefined | false>
): HTMLElementTagNameMap[K] {
  const el = document.createElement(tag);
  for (const [k, v] of Object.entries(attrs)) {
    if (v == null || v === false) continue;
    if (k === "class") el.className = String(v);
    else if (k === "style") el.setAttribute("style", String(v));
    else if (k === "dataset" && typeof v === "object") {
      Object.assign(el.dataset, v as Record<string, string>);
    } else if (k.startsWith("on") && typeof v === "function") {
      el.addEventListener(k.slice(2).toLowerCase(), v as EventListener);
    } else if (typeof v === "boolean") {
      if (v) el.setAttribute(k, "");
    } else {
      el.setAttribute(k, String(v));
    }
  }
  for (const c of children.flat(Infinity as 1)) {
    if (c == null || c === false) continue;
    el.append(c instanceof Node ? c : document.createTextNode(String(c)));
  }
  return el;
}

export function svg(tag: string, attrs: Record<string, string | number> = {}, ...children: Array<Node | string>): SVGElement {
  const el = document.createElementNS("http://www.w3.org/2000/svg", tag);
  for (const [k, v] of Object.entries(attrs)) {
    el.setAttribute(k, String(v));
  }
  for (const c of children) {
    if (c instanceof Node) el.append(c);
    else el.append(document.createTextNode(String(c)));
  }
  return el as SVGElement;
}

export function clear(el: Element) {
  while (el.firstChild) el.removeChild(el.firstChild);
}
