import { deviceInfo } from "../state/store";
import { activeProductModeVariant } from "../state/productMode";
import type { FirmwareCapabilities } from "../api/types";

export function firmwareCapabilities(): FirmwareCapabilities | undefined {
  return deviceInfo.get()?.capabilities?.firmware_capabilities;
}

function overlayVariant() {
  return activeProductModeVariant();
}

export function hasSurplusRegulation(): boolean {
  const v = overlayVariant();
  if (v) return v.caps.surplus_regulation;
  return firmwareCapabilities()?.surplus_regulation === true;
}

/** Bottom nav « Actions » — surplus triac routing on Router. */
export function showActionsNav(): boolean {
  return hasSurplusRegulation();
}

export function hasTriac(): boolean {
  return hasSurplusRegulation();
}

export function hasTriacRouting(_loadProfile?: string): boolean {
  return hasSurplusRegulation();
}

export function hasMultiAction(): boolean {
  const v = overlayVariant();
  if (v) return v.caps.multi_action;
  return firmwareCapabilities()?.multi_action === true;
}

export function compiledMeterWires(): string[] | undefined {
  const v = overlayVariant();
  if (v) {
    return v.meters.length > 0 ? v.meters : undefined;
  }
  const m = firmwareCapabilities()?.meters;
  return Array.isArray(m) && m.length > 0 ? m : undefined;
}

export function hasCompiledMeter(wire: string): boolean {
  const m = compiledMeterWires();
  return m ? m.includes(wire) : true;
}

export function meterPackId(): string | undefined {
  const v = overlayVariant();
  if (v) return v.pack_id === "full" ? "full" : v.pack_id;
  return firmwareCapabilities()?.meter_pack;
}

export function productProfileId(): string | undefined {
  const v = overlayVariant();
  if (v) return v.product_profile;
  return deviceInfo.get()?.capabilities?.product_profile;
}

export function isSingleMeterProfile(): boolean {
  const v = overlayVariant();
  if (v) return v.ui.single_meter;
  const m = compiledMeterWires();
  return m !== undefined && m.length === 1;
}

/** Advanced « Sources avancées » card: Pmqtt / JSY-333 / Tempo API (not PWM/triac). */
export function showAdvancedMeterSourcesSection(): boolean {
  const v = overlayVariant();
  if (v) return v.ui.show_advanced_meter_sources;
  if (isSingleMeterProfile()) return false;
  if (hasCompiledMeter("Pmqtt") || hasCompiledMeter("JsyMk333")) return true;
  return !hasCompiledMeter("Linky");
}
