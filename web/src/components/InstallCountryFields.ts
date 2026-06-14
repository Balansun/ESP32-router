import { h } from "../utils/dom";
import { buildFieldLabelRow, wrapSwitchWithHelp } from "./FieldHelp";
import type { RouterConfig } from "../api/types";
import {
  INSTALL_COUNTRIES,
  countryDisplayName,
  lookupInstallCountry,
  type InstallCountry,
} from "../data/install-countries";
import type { AppStrings } from "../i18n";

export interface InstallCountryUiState {
  country: string;
  variant: string;
  frequencyMode: "auto" | "manual";
  manualHz: 50 | 60;
  nominalV: number;
  editDefaults: boolean;
}

export function readInstallCountryState(cfg: RouterConfig): InstallCountryUiState {
  return {
    country: cfg.install_country || "FR",
    variant: cfg.install_country_variant || "",
    frequencyMode: cfg.mains_frequency_mode || "auto",
    manualHz: (cfg.mains_frequency_hz_manual === 60 ? 60 : 50) as 50 | 60,
    nominalV: cfg.mains_nominal_v ?? 230,
    editDefaults: cfg.install_country === "ZZ",
  };
}

export function applyInstallCountryToConfig(
  cfg: RouterConfig,
  st: InstallCountryUiState,
): RouterConfig {
  const row = lookupInstallCountry(st.country, st.variant);
  const next: RouterConfig = {
    ...cfg,
    install_country: st.country,
    install_country_variant: st.variant,
    mains_frequency_mode: st.frequencyMode,
    mains_frequency_hz_manual: st.manualHz,
  };
  if (st.country === "ZZ" || st.editDefaults) {
    next.mains_nominal_v = st.nominalV;
  } else if (row) {
    next.mains_nominal_v = row.defaultNominalV;
    next.mains_frequency_hz_manual = row.defaultFrequencyHz;
  }
  return next;
}

function splitLabel(T: AppStrings, c: InstallCountry): string {
  /* v8 ignore next -- only called when splitNoteKey is set */
  if (!c.splitNoteKey) return "";
  const key = c.splitNoteKey.replace("settings.", "") as keyof AppStrings["settings"];
  const v = T.settings[key as keyof typeof T.settings];
  return typeof v === "string" ? v : c.splitNoteKey;
}

export function buildInstallCountrySection(
  T: AppStrings,
  locale: string,
  initial: InstallCountryUiState,
  onChange: () => void,
): {
  section: HTMLElement;
  getState: () => InstallCountryUiState;
  setFrequencyWarning: (msg: string | null | undefined) => void;
} {
  let st = { ...initial };
  const summaryEl = h("p", { class: "field__hint" });
  const warnEl = h("p", { class: "field__error", hidden: true });

  const sorted = [...INSTALL_COUNTRIES].sort((a, b) =>
    countryDisplayName(a.iso2, locale).localeCompare(countryDisplayName(b.iso2, locale), locale),
  );

  const countrySelect = h(
    "select",
    { class: "input", id: "install_country" },
    ...sorted.map((c) => {
      const opt = h("option", { value: c.iso2 }, countryDisplayName(c.iso2, locale));
      if (c.iso2 === "ZZ") opt.textContent = T.settings.countryCustom;
      return opt;
    }),
  );
  countrySelect.value = st.country;

  const variantWrap = h("div", { class: "field", hidden: true });
  const variantLabel = buildFieldLabelRow({
    label: T.settings.installVariant,
    forId: "install_country_variant",
    helpScope: "settings",
    helpKey: "install_country_variant",
  });
  const variantSelect = h("select", { class: "input", id: "install_country_variant" });

  const advancedWrap = h("div", { class: "field", hidden: !st.editDefaults && st.country !== "ZZ" });
  const editDefaultsInput = h("input", { type: "checkbox", id: "edit_mains_defaults" });
  const editDefaultsLabel = wrapSwitchWithHelp(
    h(
      "label",
      { class: "switch", for: "edit_mains_defaults" },
      editDefaultsInput,
      h("span", { class: "switch__track", "aria-hidden": "true" }),
      h("span", {}, T.settings.editMainsDefaults),
    ),
    "settings",
    "edit_mains_defaults",
  );

  const freqModeSelect = h(
    "select",
    { class: "input", id: "mains_frequency_mode" },
    h("option", { value: "auto" }, T.settings.mainsModeAuto),
    h("option", { value: "manual" }, T.settings.mainsModeManual),
  );
  freqModeSelect.value = st.frequencyMode;

  const manualHzSelect = h(
    "select",
    { class: "input", id: "mains_frequency_hz_manual" },
    h("option", { value: "50" }, "50 Hz"),
    h("option", { value: "60" }, "60 Hz"),
  );
  manualHzSelect.value = String(st.manualHz);

  const nominalInput = h("input", {
    type: "number",
    class: "input",
    id: "mains_nominal_v",
    min: "100",
    max: "280",
    step: "1",
    value: String(st.nominalV),
  });

  function currentCountryRow(): InstallCountry | undefined {
    return INSTALL_COUNTRIES.find((c) => c.iso2 === st.country);
  }

  function refreshVariantOptions() {
    const c = currentCountryRow();
    variantSelect.replaceChildren();
    if (!c?.splitVariants?.length) {
      variantWrap.hidden = true;
      st.variant = "";
      return;
    }
    variantWrap.hidden = false;
    if (c.splitNoteKey) {
      const note = splitLabel(T, c);
      variantLabel.textContent = note || T.settings.installVariant;
    }
    for (const v of c.splitVariants) {
      variantSelect.append(
        h(
          "option",
          { value: v.variantId },
          `${v.defaultNominalV} V · ${v.defaultFrequencyHz} Hz`,
        ),
      );
    }
    if (!st.variant && c.splitVariants[0]) {
      st.variant = c.splitVariants[0].variantId;
    }
    variantSelect.value = st.variant;
  }

  function refreshSummary() {
    const row = lookupInstallCountry(st.country, st.variant);
    const v = st.country === "ZZ" || st.editDefaults ? st.nominalV : row?.defaultNominalV ?? st.nominalV;
    const hz =
      st.country === "ZZ" || st.editDefaults
        ? st.manualHz
        : row?.defaultFrequencyHz ?? st.manualHz;
    const mode =
      st.frequencyMode === "auto" ? T.settings.mainsModeAuto : T.settings.mainsModeManual;
    summaryEl.textContent = T.settings.mainsSummary
      .replace("{v}", String(v))
      .replace("{hz}", String(hz))
      .replace("{mode}", mode);
    advancedWrap.hidden = st.country !== "ZZ" && !st.editDefaults;
    nominalInput.disabled = st.country !== "ZZ" && !st.editDefaults;
    manualHzSelect.disabled = st.frequencyMode === "auto" && st.country !== "ZZ" && !st.editDefaults;
  }

  countrySelect.addEventListener("change", () => {
    st.country = countrySelect.value;
    st.variant = "";
    if (st.country === "ZZ") st.editDefaults = true;
    refreshVariantOptions();
    refreshSummary();
    onChange();
  });
  variantSelect.addEventListener("change", () => {
    st.variant = variantSelect.value;
    const v = currentCountryRow()?.splitVariants?.find((x) => x.variantId === st.variant);
    if (v) {
      st.manualHz = v.defaultFrequencyHz;
      st.nominalV = v.defaultNominalV;
      manualHzSelect.value = String(st.manualHz);
      nominalInput.value = String(st.nominalV);
    }
    refreshSummary();
    onChange();
  });
  editDefaultsInput.addEventListener("change", () => {
    st.editDefaults = editDefaultsInput.checked;
    refreshSummary();
    onChange();
  });
  freqModeSelect.addEventListener("change", () => {
    st.frequencyMode = freqModeSelect.value === "manual" ? "manual" : "auto";
    refreshSummary();
    onChange();
  });
  manualHzSelect.addEventListener("change", () => {
    st.manualHz = manualHzSelect.value === "60" ? 60 : 50;
    refreshSummary();
    onChange();
  });
  nominalInput.addEventListener("input", () => {
    st.nominalV = parseInt(nominalInput.value, 10) || 230;
    refreshSummary();
    onChange();
  });

  editDefaultsInput.checked = st.editDefaults;
  refreshVariantOptions();
  refreshSummary();

  variantWrap.append(variantLabel, variantSelect);
  advancedWrap.append(
    h("div", { class: "field" }, editDefaultsLabel),
    h(
      "div",
      { class: "field" },
      buildFieldLabelRow({
        label: T.settings.mainsNominalV,
        forId: "mains_nominal_v",
        helpScope: "settings",
        helpKey: "mains_nominal_v",
      }),
      nominalInput,
    ),
    h(
      "div",
      { class: "field" },
      buildFieldLabelRow({
        label: T.settings.mainsFrequencyMode,
        forId: "mains_frequency_mode",
        helpScope: "settings",
        helpKey: "mains_frequency_mode",
      }),
      freqModeSelect,
    ),
    h(
      "div",
      { class: "field" },
      buildFieldLabelRow({
        label: T.settings.mainsFrequencyHz,
        forId: "mains_frequency_hz_manual",
        helpScope: "settings",
        helpKey: "mains_frequency_hz_manual",
      }),
      manualHzSelect,
    ),
  );

  const section = h(
    "section",
    { class: "card" },
    h("h2", { class: "section__title" }, T.settings.sectionInstall),
    h(
      "div",
      { class: "field" },
      buildFieldLabelRow({
        label: T.settings.installCountry,
        forId: "install_country",
        helpScope: "settings",
        helpKey: "install_country",
      }),
      countrySelect,
    ),
    variantWrap,
    summaryEl,
    warnEl,
    advancedWrap,
  );

  return {
    section,
    getState: () => ({ ...st }),
    setFrequencyWarning(msg: string | null | undefined) {
      if (msg) {
        warnEl.hidden = false;
        warnEl.textContent = T.settings.mainsFreqWarning;
      } else {
        warnEl.hidden = true;
      }
    },
  };
}
