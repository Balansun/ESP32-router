import { h } from "../utils/dom";
import { buildFieldLabelRow, type FieldHelpScope } from "./FieldHelp";
import type { AppStrings } from "../i18n";
import {
  datetimeLocalValueToEpochSec,
  epochSecToDatetimeLocalValue,
} from "../utils/zonedDateTime";

export interface VacationEndFieldOpts {
  T: AppStrings;
  initialEpoch: number;
  getTimeZone: () => string;
  helpScope?: FieldHelpScope;
  helpKey?: string;
}

export interface VacationEndField {
  el: HTMLElement;
  input: HTMLInputElement;
  readEpoch: () => number;
  writeEpoch: (epoch: number) => void;
  refreshTzHint: () => void;
  validate: () => boolean;
}

export function buildVacationEndField(opts: VacationEndFieldOpts): VacationEndField {
  const { T, getTimeZone, helpScope = "settings", helpKey = "vacation_end_epoch" } = opts;
  const id = "vacation_end_epoch";

  const input = h("input", {
    type: "datetime-local",
    class: "field__input",
    id,
  }) as HTMLInputElement;

  const hintEl = h("p", { class: "field__hint" }, T.settings.vacationEndHint);
  const tzHintEl = h("p", { class: "field__hint" });
  const errorEl = h("p", { class: "field__error", hidden: true }) as HTMLParagraphElement;

  const labelEl = buildFieldLabelRow({
    label: T.settings.vacationEndAt,
    forId: id,
    helpScope,
    helpKey,
  });

  function refreshTzHint() {
    tzHintEl.textContent = T.settings.vacationEndTzHint.replace("{tz}", getTimeZone());
  }

  function writeEpoch(epoch: number) {
    input.value = epochSecToDatetimeLocalValue(epoch, getTimeZone());
    validate();
  }

  function readEpoch(): number {
    const raw = input.value.trim();
    if (!raw) return 0;
    const epoch = datetimeLocalValueToEpochSec(raw, getTimeZone());
    return epoch ?? 0;
  }

  function validate(): boolean {
    const raw = input.value.trim();
    if (!raw) {
      input.setAttribute("aria-invalid", "false");
      errorEl.hidden = true;
      errorEl.textContent = "";
      return true;
    }
    const ok = datetimeLocalValueToEpochSec(raw, getTimeZone()) !== null;
    input.setAttribute("aria-invalid", ok ? "false" : "true");
    errorEl.hidden = ok;
    errorEl.textContent = ok ? "" : T.settings.vacationEndInvalid;
    return ok;
  }

  input.addEventListener("input", validate);
  input.addEventListener("change", validate);

  writeEpoch(opts.initialEpoch);
  refreshTzHint();

  const el = h("div", { class: "field" }, labelEl, hintEl, tzHintEl, input, errorEl);

  return {
    el,
    input,
    readEpoch,
    writeEpoch,
    refreshTzHint,
    validate,
  };
}