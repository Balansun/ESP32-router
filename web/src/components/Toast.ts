import { h } from "../utils/dom";
import { isBrowserNetworkFailure } from "../api/networkFailure";
import { getStrings } from "../i18n";

let host: HTMLElement | null = null;

function ensureHost(): HTMLElement {
  if (!host || !host.isConnected) {
    host = h("div", {
      class: "toasts",
      role: "status",
      "aria-live": "polite",
      "aria-atomic": "true",
    });
    document.body.appendChild(host);
  }
  return host;
}

export type ToastKind = "info" | "success" | "warn" | "error";

function sanitizeToastMessage(message: string): string {
  const trimmed = message.trim();
  if (!trimmed) return trimmed;
  if (isBrowserNetworkFailure({ message: trimmed }) || /failed to fetch/i.test(trimmed)) {
    return getStrings().status.error;
  }
  return trimmed;
}

export function toast(message: string, kind: ToastKind = "info", durationMs = 3500) {
  const node = h(
    "div",
    { class: kind === "info" ? "toast" : `toast toast--${kind}` },
    sanitizeToastMessage(message),
  );
  ensureHost().append(node);
  const timer = setTimeout(() => {
    node.style.opacity = "0";
    node.style.transform = "translateY(10px)";
    setTimeout(() => node.remove(), 200);
  }, durationMs);
  node.addEventListener("click", () => {
    clearTimeout(timer);
    node.remove();
  });
}
