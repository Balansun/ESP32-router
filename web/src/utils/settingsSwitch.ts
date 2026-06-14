import { h } from "./dom";

/** Checkbox switch label matching `.switch` CSS (input + track + text). */
export function settingsSwitchLabel(
  input: HTMLInputElement,
  labelText: string,
): HTMLElement {
  return h(
    "label",
    { class: "switch" },
    input,
    h("span", { class: "switch__track", "aria-hidden": "true" }),
    h("span", {}, labelText),
  );
}
