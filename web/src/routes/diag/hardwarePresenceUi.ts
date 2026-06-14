import type { AppStrings } from "../../i18n/locales/en";
import type { HardwarePresenceItem, HardwarePresenceState } from "../../api/types";

export type AlertVariant = "danger" | "warn" | "success" | "muted" | "info";

export function alertVariantForState(state: HardwarePresenceState): AlertVariant {
  switch (state) {
    case "missing":
      return "danger";
    case "paused":
      return "warn";
    case "ok":
      return "success";
    case "not_applicable":
    default:
      return "muted";
  }
}

export function isRecheckable(item: HardwarePresenceItem): boolean {
  return item.state !== "not_applicable";
}

export function pinoutAnchor(id: string): string | null {
  switch (id) {
    case "zero_cross":
    case "triac_dim":
      return "5-triac-chain--zero-cross--gate-pulse";
    case "jsy_mk194":
      return "source-doc-uxix2";
    case "temp_ds18b20":
      return "6-ds18b20-sensor-1-wire--onewire-library--optional";
    default:
      return null;
  }
}

export function hardwareRoleLabel(id: string, T: AppStrings): string {
  const roles = T.diag.hardwareRole as Record<string, string>;
  return roles[id] ?? id;
}

export function hardwareStateLabel(state: HardwarePresenceState, T: AppStrings): string {
  const states = T.diag.hardwareState as Record<string, string>;
  return states[state] ?? state;
}

export function formatHardwarePin(item: HardwarePresenceItem, T: AppStrings): string {
  if (item.gpio_rx != null && item.gpio_tx != null) {
    return T.diag.hardwarePinsUart
      .replace("{rx}", String(item.gpio_rx))
      .replace("{tx}", String(item.gpio_tx));
  }
  if (item.gpio != null) {
    return T.diag.hardwarePin.replace("{gpio}", String(item.gpio));
  }
  return "";
}

export function hardwareDetailText(item: HardwarePresenceItem, T: AppStrings): string {
  const byId = T.diag.hardwareDetail as Record<string, Record<string, string> | undefined>;
  const localized = byId[item.id]?.[item.state];
  if (localized) return localized;
  return item.detail ?? "";
}

export function hardwareMetaLine(item: HardwarePresenceItem, T: AppStrings): string {
  const pin = formatHardwarePin(item, T);
  const state = hardwareStateLabel(item.state, T);
  return pin ? `${pin} · ${state}` : state;
}

export function partitionHardwareItems(items: HardwarePresenceItem[] | undefined): {
  warnings: HardwarePresenceItem[];
  ok: HardwarePresenceItem[];
  muted: HardwarePresenceItem[];
} {
  const list = items ?? [];
  const warnings: HardwarePresenceItem[] = [];
  const ok: HardwarePresenceItem[] = [];
  const muted: HardwarePresenceItem[] = [];
  for (const item of list) {
    if (item.state === "ok") {
      ok.push(item);
    } else if (item.state === "not_applicable") {
      muted.push(item);
    } else {
      warnings.push(item);
    }
  }
  return { warnings, ok, muted };
}
