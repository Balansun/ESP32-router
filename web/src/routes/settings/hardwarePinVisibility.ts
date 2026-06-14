import type { RouterConfig } from "../../api/types";
import { firmwareCapabilities, hasSurplusRegulation } from "../../utils/firmwareCaps";
import type { HardwarePinMetaEntry } from "./hardwarePinMeta";
import {
  isMeterPinActiveForSource,
  isSummaryRowActiveForSource,
  resolveSourceWireId,
  shouldShowMeterPinFilterHint,
  uartPresenceIdForSource,
  type HardwareEditorPinId,
} from "./meterPinRegistry";
import type { SourceWireId } from "./sourceRegistry";

export type { HardwareEditorPinId };

/** Mirror firmware balansun_source_logic_effective_id / balansun_effective_meter_id. */
export function resolveEffectiveMeterSource(cfg: RouterConfig): string {
  const active = (cfg.source || "NotDef").trim();
  if (resolveSourceWireId(active) === "BalansunPeer") {
    const peer = (cfg.source_data || "").trim();
    return peer.length > 0 ? peer : active;
  }
  return active;
}

export function shouldShowFilteredHint(source: string): boolean {
  return shouldShowMeterPinFilterHint(source);
}

export function isSummaryRowVisible(entry: HardwarePinMetaEntry, source: string): boolean {
  if (entry.configKey === "pin_triac_dim" || entry.configKey === "pin_zero_cross") {
    const fc = firmwareCapabilities();
    if (fc !== undefined && !hasSurplusRegulation()) return false;
  }
  return isSummaryRowActiveForSource(entry, source);
}

export function isEditorPinVisible(pinId: HardwareEditorPinId, source: string): boolean {
  if (pinId === "pin_triac_dim" || pinId === "pin_zero_cross") {
    const fc = firmwareCapabilities();
    if (fc !== undefined && !hasSurplusRegulation()) return false;
  }
  return isMeterPinActiveForSource(pinId, source);
}

/** @deprecated Use resolveSourceWireId — kept for callers importing SourceWireId mapping. */
export function asSourceWireId(source: string): SourceWireId | undefined {
  return resolveSourceWireId(source);
}

export { uartPresenceIdForSource };
