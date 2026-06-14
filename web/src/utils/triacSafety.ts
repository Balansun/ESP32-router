import type { ActionPeriod } from "../api/types";
import { openDialog } from "../components/Dialog";
import { h } from "./dom";
import { getStrings } from "../i18n";

/** True when at least one regulation period uses temp_min/temp_max gates (firmware: bound <= 100 °C). */
export function triacPeriodsHaveTemperatureGating(periods: ActionPeriod[]): boolean {
  for (const p of periods) {
    if (p.mode !== "power" && p.mode !== "on") continue;
    if (p.temp_inf_c <= 100 || p.temp_sup_c <= 100) return true;
  }
  return false;
}

/** User must confirm before POSTing triac_fixed at 100%. */
export function confirmLegionellaTriacFull(): Promise<boolean> {
  const T = getStrings();
  return new Promise((resolve) => {
    openDialog({
      title: T.actions.overrideTriacFullConfirmTitle,
      body: h("p", {}, `${T.actions.legionellaWarning} ${T.actions.legionellaWarningBody}`),
      closeOnBackdrop: false,
      actions: [
        { label: T.cancel, kind: "ghost", onClick: () => resolve(false) },
        { label: T.confirm, kind: "danger", onClick: () => resolve(true) },
      ],
    });
  });
}
