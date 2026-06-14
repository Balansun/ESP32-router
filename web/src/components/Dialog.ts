import { h } from "../utils/dom";
import { icon } from "../utils/icons";
import { getStrings } from "../i18n";

export interface DialogOptions {
  title: string;
  body: HTMLElement | string;
  /** Buttons rendered in the footer (left-to-right). */
  actions?: Array<{
    label: string;
    kind?: "primary" | "danger" | "ghost" | "default";
    onClick: () => void | Promise<void>;
    closeOnClick?: boolean;
  }>;
  closeOnBackdrop?: boolean;
}

function isPromiseLike(value: unknown): value is Promise<void> {
  return (
    value !== null &&
    typeof value === "object" &&
    typeof (value as Promise<void>).then === "function"
  );
}

/** Run dialog action onClick; only await real promises (preserves user activation for sync copy). */
function runDialogAction(
  onClick: () => void | Promise<void>,
  closeOnClick: boolean | undefined,
  close: () => void,
): void {
  try {
    const result = onClick();
    const maybeClose = () => {
      if (closeOnClick !== false) close();
    };
    if (isPromiseLike(result)) {
      result.then(maybeClose).catch((e) => console.error(e));
    } else {
      maybeClose();
    }
  } catch (e) {
    console.error(e);
  }
}

function focusableIn(root: HTMLElement): HTMLElement[] {
  return Array.from(
    root.querySelectorAll<HTMLElement>(
      'button:not([disabled]), [href], input:not([disabled]), select:not([disabled]), textarea:not([disabled]), [tabindex]:not([tabindex="-1"])',
    ),
  ).filter((el) => el.offsetParent !== null || el === document.activeElement);
}

/**
 * Open a modal dialog using the native `<dialog>` element. Resolves when
 * the dialog closes (cancelled or via close()).
 */
export function openDialog(opts: DialogOptions): { close: () => void } {
  const T = getStrings();
  const dlg = h("dialog", { class: "sheet-dialog" });
  const sheet = h("div", { class: "sheet" });
  const head = h("div", { class: "sheet__head" });
  const titleEl = h("h2", { class: "sheet__title", id: dialogId() }, opts.title);
  const closeBtn = h(
    "button",
    {
      type: "button",
      class: "icon-btn",
      "aria-label": T.close,
      onClick: () => close(),
    },
    icon("close"),
  );
  head.append(titleEl, closeBtn);
  sheet.append(head);
  const bodyEl = h(
    "div",
    {},
    typeof opts.body === "string" ? document.createTextNode(opts.body) : opts.body,
  );
  sheet.append(bodyEl);
  if (opts.actions?.length) {
    const footer = h("div", { class: "dialog-footer" });
    for (const a of opts.actions) {
      const btn = h(
        "button",
        {
          type: "button",
          class: `btn${a.kind && a.kind !== "default" ? " btn--" + a.kind : ""}`,
          onClick: () => runDialogAction(a.onClick, a.closeOnClick, close),
        },
        a.label,
      );
      footer.append(btn);
    }
    sheet.append(footer);
  }
  dlg.setAttribute("aria-labelledby", titleEl.id);
  dlg.append(sheet);
  document.body.append(dlg);

  const previousFocus =
    document.activeElement instanceof HTMLElement ? document.activeElement : null;

  function close() {
    if (dlg.open) dlg.close();
    setTimeout(() => {
      dlg.remove();
      previousFocus?.focus();
    }, 220);
  }

  const trapFocus = (ev: KeyboardEvent) => {
    if (ev.key !== "Tab") return;
    const nodes = focusableIn(sheet);
    if (nodes.length === 0) return;
    const first = nodes[0];
    const last = nodes[nodes.length - 1];
    if (ev.shiftKey && document.activeElement === first) {
      ev.preventDefault();
      last.focus();
    } else if (!ev.shiftKey && document.activeElement === last) {
      ev.preventDefault();
      first.focus();
    }
  };

  if (opts.closeOnBackdrop !== false) {
    dlg.addEventListener("click", (ev) => {
      if (ev.target === dlg) close();
    });
  }
  dlg.addEventListener("close", () => setTimeout(() => dlg.remove(), 220));
  dlg.addEventListener("keydown", trapFocus);
  if (typeof dlg.showModal === "function") dlg.showModal();
  /* v8 ignore next */
  else dlg.setAttribute("open", "");

  queueMicrotask(() => {
    const nodes = focusableIn(sheet);
    (nodes[0] ?? closeBtn).focus();
  });

  return { close };
}

let dialogCounter = 0;
function dialogId() {
  return `dlg-${++dialogCounter}`;
}
