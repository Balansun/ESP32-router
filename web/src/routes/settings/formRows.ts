import { h } from "../../utils/dom";
import { isValidIp } from "../../utils/format";
import { getStrings } from "../../i18n";
import { buildFieldLabelRow, type FieldHelpScope } from "../../components/FieldHelp";

export type FieldInputEl = HTMLInputElement | HTMLSelectElement;

export interface FieldRef<T = string> {
  el: FieldInputEl;
  read(): T;
  write(v: T): void;
  errorEl: HTMLParagraphElement;
}

export interface SelectOption {
  value: string;
  label: string;
}

export interface RowHelpOpts {
  helpScope?: FieldHelpScope;
  helpKey?: string;
}

export function validateIp(ref: FieldRef): boolean {
  const v = ref.el.value.trim();
  const ok = isValidIp(v);
  ref.el.setAttribute("aria-invalid", ok ? "false" : "true");
  ref.errorEl.hidden = ok;
  ref.errorEl.textContent = ok ? "" : getStrings().settings.badIp;
  return ok;
}

export function validateNumberRange(
  ref: FieldRef,
  min: number,
  max: number,
  message?: string,
): boolean {
  const n = Number(ref.read());
  const ok = Number.isFinite(n) && n >= min && n <= max;
  ref.el.setAttribute("aria-invalid", ok ? "false" : "true");
  ref.errorEl.hidden = ok;
  ref.errorEl.textContent = ok ? "" : message ?? getStrings().settings.badNumber;
  return ok;
}

export function focusFirstInvalid(root: HTMLElement): void {
  const el = root.querySelector<HTMLElement>('[aria-invalid="true"]');
  el?.focus();
}

export function row(
  id: string,
  label: string,
  hint: string,
  inputEl: FieldInputEl,
  help?: RowHelpOpts,
): { id: string; el: HTMLElement; ref: FieldRef } {
  const errorEl = h("p", { class: "field__error", hidden: true }) as HTMLParagraphElement;
  const labelEl =
    help?.helpScope && help?.helpKey
      ? buildFieldLabelRow({
          label,
          forId: id,
          helpScope: help.helpScope,
          helpKey: help.helpKey,
        })
      : h("label", { class: "field__label", for: id }, label);
  inputEl.id = id;
  const block = h(
    "div",
    { class: "field" },
    labelEl,
    hint && h("p", { class: "field__hint" }, hint),
    inputEl,
    errorEl,
  );
  return {
    id,
    el: block,
    ref: {
      el: inputEl,
      errorEl,
      read: () => inputEl.value,
      write: (v) => (inputEl.value = v),
    },
  };
}

export function textRow(
  id: string,
  label: string,
  value: string,
  hint: string,
  help?: RowHelpOpts,
) {
  const input = h("input", {
    type: "text",
    class: "field__input",
    value,
    spellcheck: "false",
    autocomplete: "off",
  }) as HTMLInputElement;
  return row(id, label, hint, input, help);
}

export function passwordRow(
  id: string,
  label: string,
  value: string,
  hint: string,
  help?: RowHelpOpts,
) {
  const input = h("input", {
    type: "password",
    class: "field__input",
    value,
    autocomplete: "new-password",
  }) as HTMLInputElement;
  return row(id, label, hint, input, help);
}

export function numberRow(
  id: string,
  label: string,
  value: number,
  hint: string,
  onInput?: () => void,
  help?: RowHelpOpts,
) {
  const input = h("input", {
    type: "number",
    class: "field__input",
    value: String(value),
    inputmode: "numeric",
  }) as HTMLInputElement;
  if (onInput) input.addEventListener("input", onInput);
  return row(id, label, hint, input, help);
}

export function selectRow(
  id: string,
  label: string,
  value: string,
  options: SelectOption[],
  hint: string,
  help?: RowHelpOpts,
) {
  const select = h("select", { class: "field__input" }) as HTMLSelectElement;
  for (const opt of options) {
    select.append(h("option", { value: opt.value }, opt.label));
  }
  const values = new Set(options.map((o) => o.value));
  select.value = values.has(value) ? value : (options[0]?.value ?? "");
  return row(id, label, hint, select, help);
}
