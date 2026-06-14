import { h } from "../../utils/dom";
import { api } from "../../api/client";
import { fetchLiveTemperatureTelemetry } from "../../api/temperatureTelemetry";
import type { HardwarePinMap, HardwarePresenceItem, RouterConfig } from "../../api/types";
import { getStrings } from "../../i18n";
import { toast } from "../../components/Toast";
import { openDialog } from "../../components/Dialog";
import { type FieldRef } from "./formRows";
import { settingsSection } from "./cards/section";
import {
  attachSettingsCardAutosave,
  type CardAutosaveLabels,
  type SettingsCardAutosave,
} from "./cardAutosave";
import { buildFieldLabelRow } from "../../components/FieldHelp";
import {
  resolveHardwarePinDefaults,
  type HardwarePinDefaults,
} from "./hardwarePinDefaults";
import type { BoardEnv } from "../../firmware/boardEnv";
import { localePref } from "../../state/store";
import {
  HARDWARE_PIN_SUMMARY_ROWS,
  hardwarePinoutRuntimeUrl,
  isPinCustom,
  pinMetaDocUrl,
  profileLabel,
  rowHint,
  type HardwarePinConfigKey,
} from "./hardwarePinMeta";
import {
  buildHardwarePinsSummaryTable,
  type PinLiveValues,
} from "./hardwarePinsSummaryTable";
import {
  isEditorPinVisible,
  resolveEffectiveMeterSource,
  shouldShowFilteredHint,
  type HardwareEditorPinId,
} from "./hardwarePinVisibility";
import { sourceFriendlyTitle } from "./measurementSourceSummary";

const CORE_PIN_IDS = [
  "pin_triac_dim",
  "pin_zero_cross",
  "pin_uart_rx",
  "pin_uart_tx",
  "pin_analog0",
  "pin_analog1",
  "pin_analog2",
] as const;

type CorePinId = (typeof CORE_PIN_IDS)[number];

const ALL_PIN_IDS = [...CORE_PIN_IDS, "pin_temp"] as const;

function readCorePin(
  cfg: RouterConfig,
  id: CorePinId,
  defaults: HardwarePinDefaults,
): number {
  return (cfg[id] as number | undefined) ?? defaults[id];
}

function readTempPin(cfg: RouterConfig, defaults: HardwarePinDefaults): number {
  return (cfg.pin_temp as number | undefined) ?? defaults.pin_temp;
}

function findDuplicateGpios(values: number[]): number | null {
  const counts = new Map<number, number>();
  for (const g of values) {
    if (g < 0) continue;
    counts.set(g, (counts.get(g) ?? 0) + 1);
    if ((counts.get(g) ?? 0) > 1) return g;
  }
  return null;
}

function metaForConfigKey(key: HardwarePinConfigKey) {
  return HARDWARE_PIN_SUMMARY_ROWS.find((e) => e.configKey === key);
}

function buildPinoutLink(anchor: string, T: ReturnType<typeof getStrings>): HTMLAnchorElement {
  const lang = localePref.get() === "fr" ? "fr" : "en";
  return h(
    "a",
    {
      class: "field__hint hardware-pin-doc-link",
      href: pinMetaDocUrl(anchor, lang),
      target: "_blank",
      rel: "noopener noreferrer",
    },
    T.settings.hardwarePinoutRowLink,
  ) as HTMLAnchorElement;
}

function buildCorePinRow(
  id: CorePinId,
  cfg: RouterConfig,
  defaults: HardwarePinDefaults,
  T: ReturnType<typeof getStrings>,
  onInput: () => void,
): { id: string; el: HTMLElement; ref: FieldRef; customBadge: HTMLElement } {
  const meta = metaForConfigKey(id);
  const defaultGpio = defaults[id];
  const input = h("input", {
    type: "number",
    class: "field__input",
    min: "0",
    max: "48",
    value: String(readCorePin(cfg, id, defaults)),
    inputmode: "numeric",
  }) as HTMLInputElement;
  input.addEventListener("input", onInput);

  const customBadge = h("span", {
    class: "hardware-pin-custom-badge",
    hidden: !isPinCustom(readCorePin(cfg, id, defaults), defaultGpio),
  }, T.settings.hardwareCustomBadge);

  const labelRow = h("div", { class: "hardware-pin-label-row" }, customBadge);
  if (meta) {
    labelRow.prepend(
      buildFieldLabelRow({
        label: T.settings.hardwarePins[id],
        forId: id,
        helpScope: "settings",
        helpKey: id,
      }),
    );
  }

  const errorEl = h("p", { class: "field__error", hidden: true }) as HTMLParagraphElement;
  const hint = meta ? rowHint(meta, defaultGpio, T) : T.settings.hardwareCorePinHint;
  const block = h(
    "div",
    { class: "field" },
    labelRow,
    h("p", { class: "field__hint" }, hint),
    meta ? buildPinoutLink(meta.pinoutAnchor, T) : null,
    input,
    errorEl,
  );

  return {
    id,
    el: block,
    customBadge,
    ref: {
      el: input,
      errorEl,
      read: () => input.value,
      write: (v) => {
        input.value = v;
        customBadge.hidden = !isPinCustom(parseInt(v, 10), defaultGpio);
      },
    },
  };
}

function buildTempPinRow(
  cfg: RouterConfig,
  defaults: HardwarePinDefaults,
  T: ReturnType<typeof getStrings>,
  onInput: () => void,
): { el: HTMLElement; ref: FieldRef<number>; customBadge: HTMLElement } {
  const meta = metaForConfigKey("pin_temp")!;
  const initial = readTempPin(cfg, defaults);
  const disabled = initial === -1;
  const modeSelect = h("select", { class: "field__input", id: "pin_temp_mode" }) as HTMLSelectElement;
  modeSelect.innerHTML = `<option value="enabled">${T.settings.hardwareTempEnabled}</option><option value="disabled">${T.settings.hardwareTempDisabled}</option>`;
  modeSelect.value = disabled ? "disabled" : "enabled";

  const gpioInput = h("input", {
    type: "number",
    class: "field__input",
    id: "pin_temp_gpio",
    min: "0",
    max: "48",
    value: String(disabled ? defaults.pin_temp : initial),
    inputmode: "numeric",
  }) as HTMLInputElement;
  gpioInput.hidden = disabled;
  gpioInput.addEventListener("input", onInput);

  const customBadge = h("span", {
    class: "hardware-pin-custom-badge",
    hidden: !isPinCustom(initial, defaults.pin_temp),
  }, T.settings.hardwareCustomBadge);

  const errorEl = h("p", { class: "field__error", hidden: true }) as HTMLParagraphElement;
  const labelRow = h(
    "div",
    { class: "hardware-pin-label-row" },
    buildFieldLabelRow({
      label: T.settings.hardwarePins.pin_temp,
      forId: "pin_temp_gpio",
      helpScope: "settings",
      helpKey: "pin_temp",
    }),
    customBadge,
  );

  const syncCustom = () => {
    const v = modeSelect.value === "disabled" ? -1 : parseInt(gpioInput.value, 10);
    customBadge.hidden = !isPinCustom(v, defaults.pin_temp);
  };

  modeSelect.addEventListener("change", () => {
    gpioInput.hidden = modeSelect.value === "disabled";
    syncCustom();
    onInput();
  });

  const block = h(
    "div",
    { class: "field" },
    labelRow,
    h("p", { class: "field__hint" }, rowHint(meta, defaults.pin_temp, T)),
    buildPinoutLink(meta.pinoutAnchor, T),
    h("label", { class: "field__label", for: "pin_temp_mode" }, T.settings.hardwareTempModeLabel),
    modeSelect,
    h("label", { class: "field__label", for: "pin_temp_gpio" }, T.settings.hardwareTempGpioLabel),
    gpioInput,
    errorEl,
  );

  return {
    el: block,
    customBadge,
    ref: {
      el: gpioInput,
      errorEl,
      read: () => (modeSelect.value === "disabled" ? -1 : parseInt(gpioInput.value, 10)),
      write: (v: number) => {
        if (v === -1) {
          modeSelect.value = "disabled";
          gpioInput.hidden = true;
        } else {
          modeSelect.value = "enabled";
          gpioInput.hidden = false;
          gpioInput.value = String(v);
        }
        syncCustom();
      },
    },
  };
}

function readCorePinFromMap(pins: HardwarePinMap, id: CorePinId, defaults: HardwarePinDefaults): number {
  const v = pins[id as keyof HardwarePinMap];
  return typeof v === "number" ? v : defaults[id];
}

function readTempPinFromMap(pins: HardwarePinMap, defaults: HardwarePinDefaults): number {
  return typeof pins.pin_temp === "number" ? pins.pin_temp : defaults.pin_temp;
}

function patchPinsFromConfig(cfg: RouterConfig, defaults: HardwarePinDefaults): Partial<RouterConfig> {
  const patch: Partial<RouterConfig> = {
    pin_map_reboot_pending: cfg.pin_map_reboot_pending,
  };
  for (const id of ALL_PIN_IDS) {
    (patch as Record<string, number>)[id] =
      id === "pin_temp" ? readTempPin(cfg, defaults) : readCorePin(cfg, id, defaults);
  }
  return patch;
}

function liveValuesFromRows(
  coreRows: Array<{ id: string; ref: FieldRef }>,
  tempRow: { ref: FieldRef<number> },
  pinDefaults: HardwarePinDefaults,
): PinLiveValues {
  const values: PinLiveValues = { ...pinDefaults, pin_temp_live: pinDefaults.pin_temp };
  coreRows.forEach((r) => {
    const n = parseInt(r.ref.read(), 10);
    if (!Number.isNaN(n)) (values as unknown as Record<string, number>)[r.id] = n;
  });
  values.pin_temp_live = Number(tempRow.ref.read());
  return values;
}

export interface HardwarePinsCardHandle {
  card: HTMLElement;
  saver: SettingsCardAutosave;
  setEffectiveSource: (source: string) => void;
}

export function buildHardwarePinsCard(
  cfg: RouterConfig,
  pinDefaults: HardwarePinDefaults,
  boardProfile: BoardEnv,
  hardwareItems: HardwarePresenceItem[] | undefined,
  signal: AbortSignal,
  cardLabels: CardAutosaveLabels,
  onStateChange: () => void,
  onSaved: (patch: Partial<RouterConfig>) => void,
): HardwarePinsCardHandle {
  const T = getStrings();
  const lang = localePref.get() === "fr" ? "fr" : "en";
  let effectiveSource = resolveEffectiveMeterSource(cfg);

  const rebootBanner = h("p", {
    class: "alert alert--warn",
    hidden: !cfg.pin_map_reboot_pending,
  }, T.settings.hardwareRebootBanner);

  const fallbackBanner = h("p", {
    class: "alert alert--danger",
    hidden: !cfg.pin_map_fallback_active,
  }, T.settings.hardwareFallbackBanner);

  const conflictHint = h("p", {
    class: "field__hint field__hint--error",
    hidden: true,
  });

  let summaryTable: ReturnType<typeof buildHardwarePinsSummaryTable> | null = null;
  let tempReadingC: number | null = null;

  const refreshSummary = () => {
    if (!summaryTable) return;
    const values = liveValuesFromRows(coreRows, tempRow, pinDefaults);
    values.temp_reading_c = tempReadingC;
    summaryTable.refresh(values);
  };

  const updateConflict = () => {
    const dup = findDuplicateGpios(readVisible());
    conflictHint.hidden = dup === null;
    if (dup !== null) {
      conflictHint.textContent = T.settings.hardwareConflict.replace("{gpio}", String(dup));
    }
    refreshSummary();
  };

  const applySourceVisibility = () => {
    for (const { row, wrap } of coreWraps) {
      wrap.hidden = !isEditorPinVisible(row.id as HardwareEditorPinId, effectiveSource);
    }
    tempWrap.hidden = !isEditorPinVisible("pin_temp", effectiveSource);
    filteredHint.hidden = !shouldShowFilteredHint(effectiveSource);
    filteredHint.textContent = T.settings.hardwarePinsFilteredHint.replace(
      "{source}",
      sourceFriendlyTitle(effectiveSource, T),
    );
    summaryTable?.setEffectiveSource(effectiveSource);
  };

  const coreRows = CORE_PIN_IDS.map((id) =>
    buildCorePinRow(id, cfg, pinDefaults, T, updateConflict),
  );
  const coreWraps = coreRows.map((r) => {
    const wrap = h("div", { class: "hardware-pin-editor-row" }) as HTMLElement;
    wrap.append(r.el);
    return { row: r, wrap };
  });
  const tempRow = buildTempPinRow(cfg, pinDefaults, T, updateConflict);
  const tempWrap = h("div", { class: "hardware-pin-editor-row" }) as HTMLElement;
  tempWrap.append(tempRow.el);

  summaryTable = buildHardwarePinsSummaryTable(
    liveValuesFromRows(coreRows, tempRow, pinDefaults),
    pinDefaults,
    hardwareItems,
    effectiveSource,
  );

  const filteredHint = h("p", {
    class: "field__hint hardware-pins-filtered-hint",
    hidden: !shouldShowFilteredHint(effectiveSource),
  }, T.settings.hardwarePinsFilteredHint.replace(
    "{source}",
    sourceFriendlyTitle(effectiveSource, T),
  ));

  applySourceVisibility();

  const profileHeader = h(
    "div",
    { class: "hardware-pins-header row" },
    h("span", { class: "status-led-chip-badge" }, profileLabel(boardProfile, T)),
    h(
      "a",
      {
        class: "field__hint hardware-pin-doc-link",
        href: hardwarePinoutRuntimeUrl(lang),
        target: "_blank",
        rel: "noopener noreferrer",
      },
      T.settings.hardwarePinoutSectionLink,
    ),
  );

  const readVisible = (): number[] => {
    const out: number[] = [];
    for (const { row, wrap } of coreWraps) {
      if (wrap.hidden) continue;
      const v = Number(row.ref.read());
      out.push(Number.isNaN(v) ? -999 : v);
    }
    if (!tempWrap.hidden) {
      const v = Number(tempRow.ref.read());
      out.push(Number.isNaN(v) ? -999 : v);
    }
    return out;
  };

  const setEffectiveSource = (source: string) => {
    effectiveSource = source;
    applySourceVisibility();
    updateConflict();
  };

  const resetBtn = h(
    "button",
    { type: "button", class: "btn btn--ghost" },
    T.settings.hardwareResetPins,
  ) as HTMLButtonElement;

  resetBtn.addEventListener("click", () => {
    openDialog({
      title: T.settings.hardwareResetConfirmTitle,
      body: h("p", {}, T.settings.hardwareResetConfirmBody),
      actions: [
        { label: T.cancel, kind: "ghost", onClick: () => {} },
        {
          label: T.settings.hardwareResetPins,
          kind: "danger",
          onClick: async () => {
            resetBtn.disabled = true;
            try {
              await api.resetPins({ signal });
              const freshPins = await api.getSystemPins({ signal });
              const resetCfg: RouterConfig = {
                ...cfg,
                board_pin_profile: freshPins.board_pin_profile ?? cfg.board_pin_profile,
                pin_default_triac_dim: freshPins.pin_default_triac_dim,
                pin_default_zero_cross: freshPins.pin_default_zero_cross,
                pin_default_uart_rx: freshPins.pin_default_uart_rx,
                pin_default_uart_tx: freshPins.pin_default_uart_tx,
                pin_default_temp: freshPins.pin_default_temp,
                pin_default_analog0: freshPins.pin_default_analog0,
                pin_default_analog1: freshPins.pin_default_analog1,
                pin_default_analog2: freshPins.pin_default_analog2,
              };
              const nextDefaults = resolveHardwarePinDefaults(resetCfg, boardProfile);
              for (const r of coreRows) {
                r.ref.write(String(readCorePinFromMap(freshPins, r.id as CorePinId, nextDefaults)));
              }
              tempRow.ref.write(readTempPinFromMap(freshPins, nextDefaults));
              updateConflict();
              toast(T.settings.hardwareResetOk, "success");
              rebootBanner.hidden = false;
              onSaved({
                ...patchPinsFromConfig(
                  {
                    ...cfg,
                    pin_triac_dim: freshPins.pin_triac_dim,
                    pin_zero_cross: freshPins.pin_zero_cross,
                    pin_uart_rx: freshPins.pin_uart_rx,
                    pin_uart_tx: freshPins.pin_uart_tx,
                    pin_temp: freshPins.pin_temp,
                    pin_analog0: freshPins.pin_analog0,
                    pin_analog1: freshPins.pin_analog1,
                    pin_analog2: freshPins.pin_analog2,
                    pin_map_reboot_pending: true,
                  },
                  nextDefaults,
                ),
                pin_map_reboot_pending: true,
              });
            } catch {
              toast(T.saveError, "error");
            } finally {
              resetBtn.disabled = false;
            }
          },
        },
      ],
    });
  });

  const card = settingsSection(
    T.settings.sectionHardwarePins,
    profileHeader,
    filteredHint,
    fallbackBanner,
    rebootBanner,
    summaryTable.el,
    h("h3", { class: "section__subtitle" }, T.settings.hardwareEditorsTitle),
    conflictHint,
    ...coreWraps.map((c) => c.wrap),
    tempWrap,
    h("div", { class: "row", style: "margin-top:12px" }, resetBtn),
  );

  const pollLive = window.setInterval(() => {
    void Promise.all([
      fetchLiveTemperatureTelemetry({ signal }),
      api.getHealth({ signal }).catch(() => null),
    ]).then(([live, health]) => {
      if (typeof live.temperature_c === "number") {
        tempReadingC = live.temperature_c;
      } else if (live.temperature_sensors?.sensors) {
        const primary = live.temperature_sensors.sensors.find((s) => s.primary && s.valid);
        const firstValid = live.temperature_sensors.sensors.find((s) => s.valid);
        const reading = primary ?? firstValid;
        tempReadingC =
          reading && typeof reading.temperature_c === "number" ? reading.temperature_c : null;
      }
      if (health?.hardware?.items) {
        summaryTable?.setHardwareItems(health.hardware.items);
      }
      refreshSummary();
    });
  }, 5000);
  signal.addEventListener("abort", () => window.clearInterval(pollLive), { once: true });

  const saver = attachSettingsCardAutosave({
    signal,
    card,
    labels: cardLabels,
    onStateChange,
    validate: () => {
      const dup = findDuplicateGpios(readVisible());
      if (dup !== null) {
        toast(T.settings.hardwareConflict.replace("{gpio}", String(dup)), "error");
        return false;
      }
      return true;
    },
    collect: () => {
      const patch: Partial<RouterConfig> = {};
      coreWraps.forEach(({ row, wrap }, i) => {
        if (wrap.hidden) return;
        (patch as Record<string, number>)[CORE_PIN_IDS[i]!] = parseInt(row.ref.read(), 10);
      });
      if (!tempWrap.hidden) {
        patch.pin_temp = tempRow.ref.read();
      }
      return patch;
    },
    persist: async (patch) => {
      const res = await api.putSystemPins(patch as Partial<HardwarePinMap>, { signal, retry: 1 });
      const merged: Partial<RouterConfig> = { ...patch };
      if (res.reboot_required) merged.pin_map_reboot_pending = true;
      onSaved(merged);
      if (res.reboot_required) rebootBanner.hidden = false;
    },
    onSaved: () => {},
  });

  return { card, saver, setEffectiveSource };
}
