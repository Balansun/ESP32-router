import { h } from "../utils/dom";
import { buildFieldLabelRow } from "./FieldHelp";
import type { AppStrings } from "../i18n";

export type PmqttPresetId = "simple_pw" | "house_snapshot" | "custom";

const PRESET_SCHEMA: Record<Exclude<PmqttPresetId, "custom">, string> = {
  simple_pw: "Pw,Pf",
  house_snapshot: "house",
};

export function pmqttSchemaForPreset(preset: PmqttPresetId, customSchema: string): string {
  if (preset === "custom") return customSchema.trim() || "Pw";
  return PRESET_SCHEMA[preset];
}

export function detectPmqttPreset(schema: string): PmqttPresetId {
  const s = schema.trim();
  if (s === PRESET_SCHEMA.simple_pw || s === "Pw") return "simple_pw";
  if (s.includes("house") || s === PRESET_SCHEMA.house_snapshot) return "house_snapshot";
  return "custom";
}

export function buildPmqttPresetFields(
  T: AppStrings,
  initialSchema: string,
  onChange: () => void,
): {
  sectionRows: HTMLElement[];
  getSchema: () => string;
} {
  let preset = detectPmqttPreset(initialSchema);
  const hintEl = h("p", { class: "field__hint" });

  const customInput = h("input", {
    class: "input",
    id: "pmqtt_schema_custom",
    type: "text",
    value: preset === "custom" ? initialSchema : "",
    hidden: preset !== "custom",
    onInput: () => onChange(),
  }) as HTMLInputElement;

  const customWrap = h(
    "div",
    { class: "field", hidden: preset !== "custom" },
    buildFieldLabelRow({
      label: T.settings.pmqttSchemaCustom,
      forId: "pmqtt_schema_custom",
      helpScope: "settings",
      helpKey: "pmqtt_schema_custom",
    }),
    customInput,
  );

  const select = h(
    "select",
    {
      class: "input",
      id: "pmqtt_preset",
      onChange: () => {
        preset = select.value as PmqttPresetId;
        customWrap.hidden = preset !== "custom";
        customInput.hidden = preset !== "custom";
        hintEl.textContent =
          preset === "simple_pw"
            ? T.settings.pmqttPresetSimpleHint
            : preset === "house_snapshot"
              ? T.settings.pmqttPresetHouseHint
              : T.settings.pmqttPresetCustomHint;
        onChange();
      },
    },
    h("option", { value: "simple_pw" }, T.settings.pmqttPresetSimple),
    h("option", { value: "house_snapshot" }, T.settings.pmqttPresetHouse),
    h("option", { value: "custom" }, T.settings.pmqttPresetCustom),
  ) as HTMLSelectElement;

  select.value = preset;
  hintEl.textContent =
    preset === "simple_pw"
      ? T.settings.pmqttPresetSimpleHint
      : preset === "house_snapshot"
        ? T.settings.pmqttPresetHouseHint
        : T.settings.pmqttPresetCustomHint;

  return {
    sectionRows: [
      h(
        "div",
        { class: "field" },
        buildFieldLabelRow({
          label: T.settings.pmqttPreset,
          forId: "pmqtt_preset",
          helpScope: "settings",
          helpKey: "pmqtt_preset",
        }),
        select,
      ),
      hintEl,
      customWrap,
    ],
    getSchema: () =>
      pmqttSchemaForPreset(select.value as PmqttPresetId, customInput.value),
  };
}
