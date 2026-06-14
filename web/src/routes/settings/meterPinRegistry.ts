/**
 * Registry + Specification: each meter source declares GPIO groups it needs;
 * pin customization shows only pins for the active source that is compiled in firmware.
 */
import { hasCompiledMeter, compiledMeterWires } from "../../utils/firmwareCaps";
import type { HardwarePinMetaEntry } from "./hardwarePinMeta";
import { registryEntry, resolveSourceWireId, type MeterGpioGroup } from "./sourceRegistry";

export type HardwareEditorPinId =
  | "pin_triac_dim"
  | "pin_zero_cross"
  | "pin_uart_rx"
  | "pin_uart_tx"
  | "pin_temp"
  | "pin_analog0"
  | "pin_analog1"
  | "pin_analog2";

export { resolveSourceWireId };

export function meterGpioGroupsForSource(source: string): MeterGpioGroup[] {
  const wireId = resolveSourceWireId(source);
  if (!wireId) return [];
  return registryEntry(wireId)?.meterGpio ?? [];
}

/** True when firmware_capabilities.meters is absent (fail-open) or includes this source. */
export function isMeterCompiledForSource(source: string): boolean {
  const wireId = resolveSourceWireId(source);
  if (!wireId) return false;
  const meters = compiledMeterWires();
  if (meters === undefined) return true;
  return hasCompiledMeter(wireId);
}

export function uartPresenceIdForSource(source: string): string | undefined {
  const wireId = resolveSourceWireId(source);
  if (!wireId) return undefined;
  return registryEntry(wireId)?.uartPresenceId;
}

function pinGpioGroup(pinId: HardwareEditorPinId): MeterGpioGroup | "router" | "temp" | null {
  if (pinId === "pin_triac_dim" || pinId === "pin_zero_cross") return "router";
  if (pinId === "pin_temp") return "temp";
  if (pinId === "pin_uart_rx" || pinId === "pin_uart_tx") return "uart";
  if (pinId === "pin_analog0" || pinId === "pin_analog1" || pinId === "pin_analog2") {
    return "analog";
  }
  return null;
}

/** Active source requires this GPIO pin and the meter is compiled into firmware. */
export function isMeterPinActiveForSource(pinId: HardwareEditorPinId, source: string): boolean {
  const group = pinGpioGroup(pinId);
  if (group === "temp") return true;
  if (group === "router" || group === null) return true;
  if (!isMeterCompiledForSource(source)) return false;
  const groups = meterGpioGroupsForSource(source);
  return groups.includes(group);
}

export function isSummaryRowActiveForSource(entry: HardwarePinMetaEntry, source: string): boolean {
  if (entry.kind === "temp") return true;
  if (entry.configKey === "pin_triac_dim" || entry.configKey === "pin_zero_cross") return true;
  if (entry.kind === "uart") {
    return isMeterCompiledForSource(source) && meterGpioGroupsForSource(source).includes("uart");
  }
  if (
    entry.configKey === "pin_analog0" ||
    entry.configKey === "pin_analog1" ||
    entry.configKey === "pin_analog2"
  ) {
    return isMeterCompiledForSource(source) && meterGpioGroupsForSource(source).includes("analog");
  }
  return true;
}

export function shouldShowMeterPinFilterHint(source: string): boolean {
  const groups = meterGpioGroupsForSource(source);
  return groups.length > 0 && groups.length < 2;
}
