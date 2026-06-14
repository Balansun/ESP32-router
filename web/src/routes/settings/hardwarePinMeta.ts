import type { AppStrings } from "../../i18n/locales/en";
import type { HardwarePresenceItem } from "../../api/types";
import { docsPageUrl } from "../../fieldHelp/docUrl";
import { fmtTempC, isProbeTemperatureReading } from "../../utils/format";
import { hardwareStateLabel } from "../diag/hardwarePresenceUi";
import type { HardwarePinDefaults } from "./hardwarePinDefaults";
import type { BoardEnv } from "../../firmware/boardEnv";

export type HardwarePinConfigKey =
  | "pin_triac_dim"
  | "pin_zero_cross"
  | "pin_uart_rx"
  | "pin_uart_tx"
  | "pin_temp"
  | "pin_analog0"
  | "pin_analog1"
  | "pin_analog2";

export type SummaryRowKind = "pin" | "uart" | "temp";

export interface HardwarePinMetaEntry {
  kind: SummaryRowKind;
  configKey?: HardwarePinConfigKey;
  labelKey: keyof AppStrings["settings"]["hardwarePins"] | "pin_uart_pair";
  roleHintKey: keyof AppStrings["settings"];
  pinoutAnchor: string;
  presenceId?: string;
  staticStatusKey?: "hardwareStatusAdc" | "hardwareStatusDisabled";
}

/** Table + editor row order (UART merged). */
export const HARDWARE_PIN_SUMMARY_ROWS: HardwarePinMetaEntry[] = [
  {
    kind: "pin",
    configKey: "pin_triac_dim",
    labelKey: "pin_triac_dim",
    roleHintKey: "hardwarePinRoleTriac",
    pinoutAnchor: "5-triac-chain--zero-cross--gate-pulse",
    presenceId: "triac_dim",
  },
  {
    kind: "pin",
    configKey: "pin_zero_cross",
    labelKey: "pin_zero_cross",
    roleHintKey: "hardwarePinRoleZeroCross",
    pinoutAnchor: "5-triac-chain--zero-cross--gate-pulse",
    presenceId: "zero_cross",
  },
  {
    kind: "uart",
    labelKey: "pin_uart_pair",
    roleHintKey: "hardwarePinRoleUart",
    pinoutAnchor: "4-esp32-jsy-serial-link-ttl-mk-194t-jsymk194-and-mk-333-jsymk333",
    presenceId: "jsy_mk194",
  },
  {
    kind: "temp",
    configKey: "pin_temp",
    labelKey: "pin_temp",
    roleHintKey: "hardwarePinRoleTemp",
    pinoutAnchor: "6-ds18b20-sensor-1-wire--onewire-library--optional",
    presenceId: "temp_ds18b20",
    staticStatusKey: "hardwareStatusDisabled",
  },
  {
    kind: "pin",
    configKey: "pin_analog0",
    labelKey: "pin_analog0",
    roleHintKey: "hardwarePinRoleAnalog",
    pinoutAnchor: "source_analog",
    staticStatusKey: "hardwareStatusAdc",
  },
  {
    kind: "pin",
    configKey: "pin_analog1",
    labelKey: "pin_analog1",
    roleHintKey: "hardwarePinRoleAnalog",
    pinoutAnchor: "source_analog",
    staticStatusKey: "hardwareStatusAdc",
  },
  {
    kind: "pin",
    configKey: "pin_analog2",
    labelKey: "pin_analog2",
    roleHintKey: "hardwarePinRoleAnalog",
    pinoutAnchor: "source_analog",
    staticStatusKey: "hardwareStatusAdc",
  },
];

export function hardwarePinoutRuntimeAnchor(lang: "en" | "fr"): string {
  return lang === "fr"
    ? "assignation-gpio-a-lexecution-sans-reflash"
    : "runtime-gpio-assignment-no-reflash";
}

export function hardwarePinoutRuntimeUrl(lang: "en" | "fr"): string {
  return docsPageUrl(`/${lang}/build/pinout/gpio-and-outputs#${hardwarePinoutRuntimeAnchor(lang)}`);
}

export function pinMetaDocUrl(anchor: string, lang: "en" | "fr"): string {
  if (anchor === "source_analog") {
    return docsPageUrl(`/${lang}/build/pinout/sources/analog#source_analog`);
  }
  return docsPageUrl(`/${lang}/build/pinout/gpio-and-outputs#${anchor}`);
}

export function profileLabel(boardProfile: BoardEnv, T: AppStrings): string {
  return boardProfile === "esp32s3" ? T.settings.hardwareProfileS3 : T.settings.hardwareProfileWroom;
}

export function lookupPresence(
  items: HardwarePresenceItem[] | undefined,
  id: string | undefined,
): HardwarePresenceItem | undefined {
  if (!items || !id) return undefined;
  return items.find((item) => item.id === id);
}

export function isPinCustom(value: number, defaultGpio: number): boolean {
  if (value === -1) return true;
  return value !== defaultGpio;
}

export function formatPinStatus(
  item: HardwarePresenceItem | undefined,
  staticKey: HardwarePinMetaEntry["staticStatusKey"],
  tempDisabled: boolean,
  T: AppStrings,
  opts?: { entryKind?: SummaryRowKind; tempReadingC?: number | null },
): string {
  if (staticKey === "hardwareStatusDisabled" && tempDisabled) {
    return T.settings.hardwareStatusDisabled;
  }
  if (staticKey === "hardwareStatusAdc") {
    return T.settings.hardwareStatusAdc;
  }
  if (!item) return T.settings.hardwareStatusNa;
  if (
    opts?.entryKind === "temp" &&
    item.state === "ok" &&
    isProbeTemperatureReading(opts.tempReadingC)
  ) {
    return `${hardwareStateLabel("ok", T)} · ${fmtTempC(opts.tempReadingC)}`;
  }
  return hardwareStateLabel(item.state, T);
}

export function statusChipClass(
  item: HardwarePresenceItem | undefined,
  staticKey: HardwarePinMetaEntry["staticStatusKey"],
  tempDisabled: boolean,
): string {
  if (staticKey === "hardwareStatusAdc") return "hardware-pin-chip hardware-pin-chip--muted";
  if (staticKey === "hardwareStatusDisabled" && tempDisabled) {
    return "hardware-pin-chip hardware-pin-chip--muted";
  }
  if (!item) return "hardware-pin-chip hardware-pin-chip--muted";
  switch (item.state) {
    case "ok":
      return "hardware-pin-chip hardware-pin-chip--ok";
    case "missing":
      return "hardware-pin-chip hardware-pin-chip--danger";
    case "paused":
      return "hardware-pin-chip hardware-pin-chip--warn";
    default:
      return "hardware-pin-chip hardware-pin-chip--muted";
  }
}

export function rowLabel(entry: HardwarePinMetaEntry, T: AppStrings): string {
  if (entry.labelKey === "pin_uart_pair") return T.settings.hardwarePins.pin_uart_pair;
  return T.settings.hardwarePins[entry.labelKey];
}

export function rowHint(entry: HardwarePinMetaEntry, defaultGpio: number, T: AppStrings): string {
  const role = T.settings[entry.roleHintKey] as string;
  if (entry.kind === "temp") {
    return `${role} ${T.settings.hardwareDefaultGpio.replace("{gpio}", String(defaultGpio))}`;
  }
  if (entry.kind === "uart") {
    return role;
  }
  return `${role} ${T.settings.hardwareDefaultGpio.replace("{gpio}", String(defaultGpio))}`;
}

export function gpioCellText(
  entry: HardwarePinMetaEntry,
  values: Readonly<HardwarePinDefaults> & { pin_temp_live: number },
): string {
  if (entry.kind === "uart") {
    return `RX ${values.pin_uart_rx} · TX ${values.pin_uart_tx}`;
  }
  if (entry.kind === "temp") {
    return values.pin_temp_live === -1 ? "—" : String(values.pin_temp_live);
  }
  if (entry.configKey) {
    return String(values[entry.configKey]);
  }
  return "—";
}

export function defaultCellText(entry: HardwarePinMetaEntry, defaults: HardwarePinDefaults): string {
  if (entry.kind === "uart") {
    return `RX ${defaults.pin_uart_rx} · TX ${defaults.pin_uart_tx}`;
  }
  if (entry.kind === "temp") {
    return String(defaults.pin_temp);
  }
  if (entry.configKey) {
    return String(defaults[entry.configKey]);
  }
  return "—";
}

export function rowIsCustom(
  entry: HardwarePinMetaEntry,
  values: Readonly<HardwarePinDefaults> & { pin_temp_live: number },
  defaults: HardwarePinDefaults,
): boolean {
  if (entry.kind === "uart") {
    return (
      values.pin_uart_rx !== defaults.pin_uart_rx || values.pin_uart_tx !== defaults.pin_uart_tx
    );
  }
  if (entry.kind === "temp") {
    return isPinCustom(values.pin_temp_live, defaults.pin_temp);
  }
  if (entry.configKey) {
    return isPinCustom(values[entry.configKey], defaults[entry.configKey]);
  }
  return false;
}
