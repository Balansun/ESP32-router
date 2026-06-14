import { h } from "../../../utils/dom";
import { api } from "../../../api/client";
import { toast } from "../../../components/Toast";
import { openDialog } from "../../../components/Dialog";
import { openFactoryResetDialog } from "../../../components/FactoryResetDialog";
import type { getStrings } from "../../../i18n";
import type { buildTimezoneCountryField } from "../../../components/TimezoneCountryField";
import type { textRow } from "../formRows";

export function buildAdvancedSection(
  T: ReturnType<typeof getStrings>,
  signal: AbortSignal,
  timeNowEl: HTMLElement,
  tzField: ReturnType<typeof buildTimezoneCountryField>,
  ntp1Field: ReturnType<typeof textRow>,
  ntp2Field: ReturnType<typeof textRow>,
  extraRows?: HTMLElement,
): HTMLElement {
  async function runAction(
    fn: () => Promise<unknown>,
    okMsg = T.settings.actionOk,
  ) {
    try {
      await fn();
      toast(okMsg, "success");
    } catch {
      toast(T.settings.actionFailed, "error");
    }
  }

  return h(
    "section",
    { class: "card" },
    h("h2", { class: "section__title" }, T.settings.sectionAdvanced),
    timeNowEl,
    tzField.el,
    ntp1Field.el,
    ntp2Field.el,
    ...(extraRows ? [extraRows] : []),
    h(
      "div",
      { class: "row", style: "gap:8px;flex-wrap:wrap;margin-top:8px;" },
      h(
        "button",
        {
          type: "button",
          class: "btn btn--ghost",
          onClick: () => runAction(() => api.saveNow({ signal })),
        },
        T.settings.saveEeprom,
      ),
      h(
        "button",
        {
          type: "button",
          class: "btn btn--ghost",
          onClick: () =>
            openDialog({
              title: T.settings.resetHistory,
              body: h("p", {}, T.settings.resetHistoryConfirm),
              actions: [
                { label: T.cancel, kind: "ghost", onClick: () => {} },
                {
                  label: T.confirm,
                  kind: "danger",
                  onClick: () => runAction(() => api.resetHistory({ signal })),
                },
              ],
            }),
        },
        T.settings.resetHistory,
      ),
      h(
        "button",
        {
          type: "button",
          class: "btn btn--danger",
          onClick: () => openFactoryResetDialog({ signal }),
        },
        T.settings.factoryReset,
      ),
    ),
  );
}
