import { h } from "../../utils/dom";
import type { HardwarePresenceItem } from "../../api/types";
import { getStrings } from "../../i18n";
import type { HardwarePinDefaults } from "./hardwarePinDefaults";
import {
  defaultCellText,
  gpioCellText,
  HARDWARE_PIN_SUMMARY_ROWS,
  rowIsCustom,
  rowLabel,
  formatPinStatus,
  lookupPresence,
  statusChipClass,
  type HardwarePinMetaEntry,
} from "./hardwarePinMeta";
import { isSummaryRowVisible, uartPresenceIdForSource } from "./hardwarePinVisibility";

export interface PinLiveValues extends HardwarePinDefaults {
  pin_temp_live: number;
  temp_reading_c?: number | null;
}

export interface HardwarePinsSummaryTableHandle {
  el: HTMLElement;
  refresh(values: PinLiveValues): void;
  setEffectiveSource(source: string): void;
  setHardwareItems(items: HardwarePresenceItem[] | undefined): void;
}

function buildRowCells(
  entry: HardwarePinMetaEntry,
  values: PinLiveValues,
  defaults: HardwarePinDefaults,
  hardwareItems: HardwarePresenceItem[] | undefined,
  effectiveSource: string,
): HTMLTableRowElement {
  const T = getStrings();
  const tr = h("tr", {}) as HTMLTableRowElement;
  const custom = rowIsCustom(entry, values, defaults);
  const tempDisabled = entry.kind === "temp" && values.pin_temp_live === -1;
  const presenceId =
    entry.kind === "uart" ? uartPresenceIdForSource(effectiveSource) ?? entry.presenceId : entry.presenceId;
  const presence = lookupPresence(hardwareItems, presenceId);
  const statusText = formatPinStatus(presence, entry.staticStatusKey, tempDisabled, T, {
    entryKind: entry.kind,
    tempReadingC: values.temp_reading_c,
  });

  const fnCell = h("td", {}, rowLabel(entry, T));
  if (custom) {
    fnCell.append(
      h("span", { class: "hardware-pin-custom-badge" }, T.settings.hardwareCustomBadge),
    );
  }

  tr.append(
    fnCell,
    h("td", { class: "num" }, gpioCellText(entry, values)),
    h("td", { class: "num" }, defaultCellText(entry, defaults)),
    h(
      "td",
      {},
      h("span", { class: statusChipClass(presence, entry.staticStatusKey, tempDisabled) }, statusText),
    ),
  );
  return tr;
}

export function buildHardwarePinsSummaryTable(
  initialValues: PinLiveValues,
  defaults: HardwarePinDefaults,
  hardwareItems: HardwarePresenceItem[] | undefined,
  effectiveSource: string,
): HardwarePinsSummaryTableHandle {
  const T = getStrings();
  const tbody = h("tbody", {}) as HTMLTableSectionElement;
  let source = effectiveSource;
  let lastValues = initialValues;
  let presenceItems = hardwareItems;

  const visibleRows = () =>
    HARDWARE_PIN_SUMMARY_ROWS.filter((entry) => isSummaryRowVisible(entry, source));

  const refresh = (values: PinLiveValues) => {
    lastValues = values;
    tbody.replaceChildren(
      ...visibleRows().map((entry) =>
        buildRowCells(entry, values, defaults, presenceItems, source),
      ),
    );
  };
  refresh(initialValues);

  const setEffectiveSource = (next: string) => {
    source = next;
    refresh(lastValues);
  };

  const setHardwareItems = (items: HardwarePresenceItem[] | undefined) => {
    presenceItems = items;
    refresh(lastValues);
  };

  const table = h(
    "div",
    { class: "table-wrap hardware-pins-summary" },
    h("h3", { class: "section__subtitle" }, T.settings.hardwareSummaryTitle),
    h(
      "table",
      { class: "table" },
      h(
        "thead",
        {},
        h(
          "tr",
          {},
          h("th", {}, T.settings.hardwareColFunction),
          h("th", { class: "num" }, T.settings.hardwareColGpio),
          h("th", { class: "num" }, T.settings.hardwareColDefault),
          h("th", {}, T.settings.hardwareColStatus),
        ),
      ),
      tbody,
    ),
  );

  return { el: table, refresh, setEffectiveSource, setHardwareItems };
}
