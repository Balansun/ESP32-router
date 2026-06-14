import { api } from "../api/client";
import type { HardwarePresenceItem } from "../api/types";
import { getStrings } from "../i18n";
import { pinoutSectionUrl } from "../fieldHelp/docUrl";
import { localePref } from "../state/store";
import { h } from "../utils/dom";
import { icon } from "../utils/icons";
import { toast } from "./Toast";
import {
  alertVariantForState,
  hardwareDetailText,
  hardwareMetaLine,
  hardwareRoleLabel,
  isRecheckable,
  partitionHardwareItems,
  pinoutAnchor,
  type AlertVariant,
} from "../routes/diag/hardwarePresenceUi";

export interface HardwarePresencePanelHandle {
  el: HTMLElement;
  setItems(items: HardwarePresenceItem[] | undefined): void;
}

function buildAlert(
  item: HardwarePresenceItem,
  variant: AlertVariant,
  opts: {
    compact?: boolean;
    onRecheck?: (id: string, el: HTMLElement) => void;
  },
): HTMLElement {
  const T = getStrings();
  const alert = h("div", {
    class: `alert alert--${variant}${opts.compact ? " alert--compact" : ""}`,
    role: "alert",
    "data-hw-id": item.id,
  });
  const head = h("div", { class: "alert__head" });
  if (!opts.compact && (variant === "danger" || variant === "warn")) {
    head.append(h("span", { class: "alert__icon" }, icon("alert")));
  }
  head.append(
    h("strong", { class: "alert__title" }, hardwareRoleLabel(item.id, T)),
    h("span", { class: "alert__meta" }, hardwareMetaLine(item, T)),
  );
  alert.append(
    head,
    h("p", { class: "alert__body" }, hardwareDetailText(item, T)),
  );

  const actions = h("div", { class: "alert__actions" });
  let hasActions = false;
  if (opts.onRecheck && isRecheckable(item) && !opts.compact) {
    hasActions = true;
    const btn = h(
      "button",
      {
        type: "button",
        class: "btn btn--ghost btn--sm",
        onClick: () => opts.onRecheck!(item.id, alert),
      },
      T.diag.hardwareRecheck,
    );
    actions.append(btn);
  }
  const anchor = pinoutAnchor(item.id);
  if (anchor && !opts.compact) {
    hasActions = true;
    const lang = localePref.get() === "fr" ? "fr" : "en";
    actions.append(
      h(
        "a",
        {
          class: "alert__link",
          href: pinoutSectionUrl(anchor, lang),
          target: "_blank",
          rel: "noopener noreferrer",
        },
        T.diag.hardwarePinoutLink,
      ),
    );
  }
  if (hasActions) alert.append(actions);
  return alert;
}

export function buildHardwarePresencePanel(opts: {
  signal: AbortSignal;
  onItemsChange?: (items: HardwarePresenceItem[]) => void;
}): HardwarePresencePanelHandle {
  const T = getStrings();
  let items: HardwarePresenceItem[] = [];
  let recheckingAll = false;

  const recheckAllBtn = h(
    "button",
    {
      type: "button",
      class: "btn btn--ghost btn--sm",
      onClick: () => void recheckAll(),
    },
    T.diag.hardwareRecheckAll,
  );

  const card = h(
    "section",
    { class: "card" },
    h(
      "div",
      { class: "hw-presence__head" },
      h("div", {}, h("h2", { class: "card__title" }, T.diag.hardwareTitle), h("p", { class: "card__sub" }, T.diag.hardwareWarnIntro)),
      recheckAllBtn,
    ),
  );

  const warnStack = h("div", { class: "alert-stack hw-presence__warnings" });
  const mutedStack = h("div", { class: "alert-stack hw-presence__muted" });
  const okDetails = h("details", { class: "hw-presence__ok-details", hidden: true });
  const okSummary = h("summary", {});
  const okStack = h("div", { class: "alert-stack" });
  okDetails.append(okSummary, okStack);
  card.append(warnStack, mutedStack, okDetails);

  async function recheckOne(id: string, alertEl: HTMLElement) {
    alertEl.classList.add("alert--loading");
    try {
      const res = await api.postHealthHardwareRecheck(id, { signal: opts.signal });
      items = items.map((it) => (it.id === id ? res.item : it));
      opts.onItemsChange?.(items);
      render();
      toast(T.diag.hardwareRecheckDone, "success");
    } catch {
      toast(T.diag.hardwareRecheckFailed, "error");
    } finally {
      alertEl.classList.remove("alert--loading");
    }
  }

  async function recheckAll() {
    if (recheckingAll) return;
    const targets = items.filter((it) => isRecheckable(it) && it.state !== "ok");
    if (targets.length === 0) return;
    recheckingAll = true;
    recheckAllBtn.setAttribute("disabled", "true");
    try {
      for (const item of targets) {
        const res = await api.postHealthHardwareRecheck(item.id, { signal: opts.signal });
        items = items.map((it) => (it.id === item.id ? res.item : it));
      }
      opts.onItemsChange?.(items);
      render();
      toast(T.diag.hardwareRecheckDone, "success");
    } catch {
      toast(T.diag.hardwareRecheckFailed, "error");
    } finally {
      recheckingAll = false;
      recheckAllBtn.removeAttribute("disabled");
    }
  }

  function render() {
    const { warnings, ok, muted } = partitionHardwareItems(items);
    recheckAllBtn.hidden = warnings.length === 0;

    warnStack.replaceChildren(
      ...warnings.map((item) =>
        buildAlert(item, alertVariantForState(item.state), {
          onRecheck: recheckOne,
        }),
      ),
    );

    mutedStack.replaceChildren(
      ...muted.map((item) =>
        buildAlert(item, "muted", { compact: true }),
      ),
    );
    mutedStack.hidden = muted.length === 0;

    okDetails.hidden = ok.length === 0;
    okSummary.textContent = T.diag.hardwareAllOk.replace("{count}", String(ok.length));
    okStack.replaceChildren(
      ...ok.map((item) =>
        buildAlert(item, "success", { compact: true }),
      ),
    );
  }

  return {
    el: card,
    setItems(next) {
      items = next ?? [];
      render();
    },
  };
}
