import { h } from "../../utils/dom";
import type { AppStrings } from "../../i18n/locales/en";
import { deviceInfo, localePref } from "../../state/store";
import { withBase } from "../../paths";
import { settingsSection } from "./cards/section";
import { toast } from "../../components/Toast";
import {
  canSelectProductMode,
  productModePioEnv,
  setProductModePioEnv,
  syncProductModeFromDevice,
} from "../../state/productMode";
import {
  catalogEntriesForFamily,
  catalogEntryLabel,
  catalogFamilyLabel,
  entriesMatchFlashed,
  releaseFirmwareArtifactName,
  resolveFlashedCatalogEntry,
  type FirmwareCatalog,
  type FirmwareCatalogEntry,
  type ProductFamilyId,
} from "../../utils/productProfile";
import type { FirmwareCapabilities } from "../../api/types";

export interface ProductTypeSettingsInput {
  catalog: FirmwareCatalog;
  productProfile?: string;
  firmwareCapabilities?: FirmwareCapabilities;
  firmwareVersion: string;
  onModeChange?: () => void;
}

export interface ProductTypeSettingsCard {
  card: HTMLElement;
}

export function buildProductTypeSettingsCard(
  T: AppStrings,
  input: ProductTypeSettingsInput,
): ProductTypeSettingsCard {
  syncProductModeFromDevice();
  const locale = localePref.get() === "fr" ? "fr" : "en";
  const flashed = resolveFlashedCatalogEntry(
    input.catalog,
    input.firmwareCapabilities,
    input.productProfile,
  );

  const activeEnv = canSelectProductMode()
    ? productModePioEnv.get() ?? flashed?.pio_env ?? "wroom32"
    : flashed?.pio_env;
  const activeEntry = activeEnv
    ? input.catalog.entries.find((e) => e.pio_env === activeEnv)
    : flashed;

  const firmwareInfoEl = h("p", { class: "field__hint product-type__firmware" });
  const otaBanner = h("div", {
    class: "banner banner--warn product-type__ota",
    hidden: true,
  });

  const familySelect = h("select", {
    id: "product-family",
    class: "field__input",
    "aria-label": T.settings.productFamilyLabel,
    disabled: !canSelectProductMode(),
  }) as HTMLSelectElement;

  for (const fam of input.catalog.families) {
    const opt = h("option", { value: fam.id }, catalogFamilyLabel(fam, locale));
    familySelect.append(opt);
  }

  const declinationSelect = h("select", {
    id: "product-declination",
    class: "field__input",
    "aria-label": T.settings.productDeclinationLabel,
    disabled: !canSelectProductMode(),
  }) as HTMLSelectElement;

  const applyBtn = h(
    "button",
    {
      type: "button",
      id: "product-mode-apply",
      class: "btn btn--primary",
      hidden: !canSelectProductMode(),
      disabled: true,
    },
    T.settings.productApplyMode,
  ) as HTMLButtonElement;

  function fillDeclinations(familyId: ProductFamilyId, selectEnv?: string) {
    declinationSelect.replaceChildren();
    const entries = catalogEntriesForFamily(input.catalog, familyId);
    for (const entry of entries) {
      declinationSelect.append(
        h("option", { value: entry.pio_env }, catalogEntryLabel(entry, locale)),
      );
    }
    if (selectEnv && entries.some((e) => e.pio_env === selectEnv)) {
      declinationSelect.value = selectEnv;
    } else if (entries[0]) {
      declinationSelect.value = entries[0].pio_env;
    }
  }

  if (activeEntry) {
    familySelect.value = activeEntry.family;
    fillDeclinations(activeEntry.family, activeEntry.pio_env);
  } else {
    fillDeclinations("full", "wroom32");
  }

  function selectedEntry(): FirmwareCatalogEntry | undefined {
    return input.catalog.entries.find((e) => e.pio_env === declinationSelect.value);
  }

  function appliedPioEnv(): string | undefined {
    if (!canSelectProductMode()) return flashed?.pio_env;
    return productModePioEnv.get() ?? flashed?.pio_env ?? "wroom32";
  }

  function renderFirmwareInfo() {
    if (canSelectProductMode()) {
      firmwareInfoEl.textContent = T.settings.productFullFirmwareHint;
      firmwareInfoEl.hidden = false;
      return;
    }
    if (flashed) {
      const fam = input.catalog.families.find((f) => f.id === flashed.family);
      firmwareInfoEl.textContent = T.settings.productFlashedOnlyHint
        .replace("{family}", fam ? catalogFamilyLabel(fam, locale) : flashed.family)
        .replace("{declination}", catalogEntryLabel(flashed, locale));
      firmwareInfoEl.hidden = false;
    } else {
      firmwareInfoEl.hidden = true;
    }
  }

  function renderOtaBanner() {
    if (canSelectProductMode()) {
      otaBanner.hidden = true;
      otaBanner.replaceChildren();
      return;
    }
    const selected = selectedEntry();
    if (!selected || entriesMatchFlashed(flashed, selected)) {
      otaBanner.hidden = true;
      otaBanner.replaceChildren();
      return;
    }
    const artifact = releaseFirmwareArtifactName(input.firmwareVersion, selected.pio_env);
    otaBanner.hidden = false;
    otaBanner.replaceChildren(
      h("p", { class: "banner__title" }, T.settings.productOtaTitle),
      h("p", {}, T.settings.productOtaBody),
      h("p", { class: "product-type__artifact" }, h("code", {}, artifact)),
      h("p", { class: "field__hint" }, T.settings.productOtaEnv.replace("{env}", selected.pio_env)),
      h(
        "a",
        {
          href: "https://balansun.clouded.fr/en/getting-started/",
          target: "_blank",
          rel: "noopener noreferrer",
          class: "btn btn--ghost",
        },
        T.settings.productOtaDocsLink,
      ),
    );
  }

  function syncApplyButton(): void {
    if (!canSelectProductMode()) {
      applyBtn.hidden = true;
      renderOtaBanner();
      return;
    }
    applyBtn.hidden = false;
    const pending = declinationSelect.value;
    const applied = appliedPioEnv();
    applyBtn.disabled = !pending || pending === applied;
  }

  function applyPendingMode(): void {
    const selected = selectedEntry();
    if (!selected || !canSelectProductMode()) return;
    if (selected.pio_env === appliedPioEnv()) return;
    setProductModePioEnv(selected.pio_env);
    input.onModeChange?.();
    syncApplyButton();
    toast(T.settings.productModeApplied, "success");
  }

  familySelect.addEventListener("change", () => {
    fillDeclinations(familySelect.value as ProductFamilyId);
    syncApplyButton();
  });
  declinationSelect.addEventListener("change", () => syncApplyButton());
  applyBtn.addEventListener("click", () => applyPendingMode());

  function syncSelectableState(): void {
    const nowSelectable = canSelectProductMode();
    familySelect.disabled = !nowSelectable;
    declinationSelect.disabled = !nowSelectable;
    renderFirmwareInfo();
    syncApplyButton();
  }

  productModePioEnv.subscribe(() => syncApplyButton());
  deviceInfo.subscribe(() => syncSelectableState());
  syncSelectableState();

  const intro = canSelectProductMode() ? T.settings.productIntroFull : T.settings.productIntro;

  const card = settingsSection(
    T.settings.sectionProductType,
    h("p", { class: "field__hint" }, intro),
    firmwareInfoEl,
    h("div", { class: "field" }, h("label", { class: "field__label", for: "product-family" }, T.settings.productFamilyLabel), familySelect),
    h(
      "div",
      { class: "field" },
      h("label", { class: "field__label", for: "product-declination" }, T.settings.productDeclinationLabel),
      declinationSelect,
    ),
    applyBtn,
    otaBanner,
    h(
      "a",
      {
        href: withBase("/firmware-catalog.json"),
        class: "field__hint",
        target: "_blank",
        rel: "noopener noreferrer",
      },
      T.settings.productCatalogLink,
    ),
  );

  return { card };
}
