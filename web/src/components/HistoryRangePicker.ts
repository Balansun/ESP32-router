import { h } from "../utils/dom";

export interface HistoryRangePickerOption {
  id: string;
  label: string;
}

export interface HistoryRangePickerHandle {
  el: HTMLElement;
  setValue(value: string): void;
  getValue(): string;
}

export function buildHistoryRangePicker(opts: {
  name: string;
  ariaLabel: string;
  value: string;
  options: HistoryRangePickerOption[];
  onChange: (value: string) => void;
}): HistoryRangePickerHandle {
  const fieldset = h("fieldset", { class: "history-range" });
  fieldset.append(h("legend", { class: "sr-only" }, opts.ariaLabel));

  let currentValue = opts.value;
  const inputs: Record<string, HTMLInputElement> = {};

  for (const opt of opts.options) {
    const input = h("input", {
      type: "radio",
      name: opts.name,
      value: opt.id,
      ...(opt.id === opts.value ? { checked: true } : {}),
    }) as HTMLInputElement;
    const label = h("label", { class: "history-range__segment" }, input, opt.label);
    input.addEventListener("change", () => {
      if (!input.checked) return;
      currentValue = opt.id;
      opts.onChange(opt.id);
    });
    inputs[opt.id] = input;
    fieldset.append(label);
  }

  return {
    el: fieldset,
    setValue(value: string) {
      currentValue = value;
      for (const [id, input] of Object.entries(inputs)) {
        input.checked = id === value;
      }
    },
    getValue: () => currentValue,
  };
}
