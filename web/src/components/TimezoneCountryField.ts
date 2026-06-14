import { h } from "../utils/dom";
import { buildFieldLabelRow } from "./FieldHelp";
import {
  INSTALL_COUNTRIES,
  countryDisplayName,
  lookupInstallCountry,
} from "../data/install-countries";
import type { AppStrings } from "../i18n";

export function tzForCountryIso(iso2: string, variant?: string): string {
  const row = lookupInstallCountry(iso2, variant);
  return row?.suggestedTimeTz ?? "UTC";
}

export function countryIsoForTz(tz: string, preferIso2?: string): string {
  const trimmed = String(tz).trim();
  if (trimmed.length > 0) {
    const pref = String(preferIso2 ?? "").trim().toUpperCase();
    if (pref.length > 0) {
      const row = lookupInstallCountry(pref);
      if (row?.suggestedTimeTz === trimmed) return pref;
    }
    const hit = INSTALL_COUNTRIES.find((c) => c.suggestedTimeTz === trimmed);
    return hit?.iso2 ?? "ZZ";
  }
  const p = preferIso2 == null ? "" : String(preferIso2).trim().toUpperCase();
  if (p.length > 0 && INSTALL_COUNTRIES.some((c) => c.iso2 === p)) return p;
  return "FR";
}

export function buildTimezoneCountryField(
  T: AppStrings,
  locale: string,
  initialTz: string,
  preferCountryIso?: string,
  onChange?: () => void,
): {
  el: HTMLElement;
  readTz: () => string;
  writeTz: (tz: string) => void;
} {
  let tzValue = (initialTz || "").trim() || tzForCountryIso("FR");
  let countryIso = countryIsoForTz(tzValue, preferCountryIso);

  const hintEl = h("p", { class: "field__hint" });

  const sorted = [...INSTALL_COUNTRIES].sort((a, c) =>
    countryDisplayName(a.iso2, locale).localeCompare(countryDisplayName(c.iso2, locale), locale),
  );

  const countrySelect = h(
    "select",
    { class: "input", id: "time_tz_country" },
    ...sorted.map((c) => {
      const opt = h("option", { value: c.iso2 }, countryDisplayName(c.iso2, locale));
      if (c.iso2 === "ZZ") opt.textContent = T.settings.countryCustom;
      return opt;
    }),
  );
  countrySelect.value = countryIso;

  function refreshHint() {
    hintEl.textContent = T.settings.tzIana.replace("{tz}", tzValue);
  }

  countrySelect.addEventListener("change", () => {
    countryIso = countrySelect.value;
    tzValue = tzForCountryIso(countryIso);
    refreshHint();
    onChange?.();
  });

  refreshHint();

  const block = h(
    "div",
    { class: "field" },
    buildFieldLabelRow({
      label: T.settings.tzCountry,
      forId: "time_tz_country",
      helpScope: "settings",
      helpKey: "tz_country",
    }),
    countrySelect,
    hintEl,
  );

  return {
    el: block,
    readTz: () => tzValue,
    writeTz: (tz: string) => {
      tzValue = (tz || "").trim() || tzForCountryIso("FR");
      countryIso = countryIsoForTz(tzValue, preferCountryIso);
      countrySelect.value = countryIso;
      refreshHint();
    },
  };
}
