import { h } from "../../utils/dom";
import { buildFieldLabelRow, type FieldHelpScope } from "../../components/FieldHelp";

export interface FieldHelpOpts {
  helpScope?: FieldHelpScope;
  helpKey?: string;
}

export function buildSwitch(
  initial: boolean,
  label: string,
  onChange: (v: boolean) => void,
): HTMLElement {
  const input = h("input", {
    type: "checkbox",
    checked: initial,
    onChange: (ev) => onChange((ev.target as HTMLInputElement).checked),
  });
  return h(
    "label",
    { class: "switch", title: label },
    input,
    h("span", { class: "switch__track", "aria-hidden": "true" }),
    h("span", { class: "sr-only" }, label),
  );
}

export function textField(
  label: string,
  hint: string,
  initial: string,
  onChange: (v: string) => void,
  type: "text" | "number" = "text",
  help?: FieldHelpOpts,
): HTMLElement {
  const input = h("input", {
    class: "field__input",
    type,
    value: initial,
    onInput: (ev) => onChange((ev.target as HTMLInputElement).value),
  });
  const labelEl =
    help?.helpScope && help?.helpKey
      ? buildFieldLabelRow({
          label,
          helpScope: help.helpScope,
          helpKey: help.helpKey,
        })
      : h("span", { class: "field__label" }, label);
  return h(
    "div",
    { class: "field" },
    labelEl,
    hint && h("p", { class: "field__hint" }, hint),
    input,
  );
}

export function numberField(
  label: string,
  initial: string,
  onChange: (v: string) => void,
  hint = "",
  help?: FieldHelpOpts,
): HTMLElement {
  return textField(label, hint, initial, onChange, "number", help);
}
