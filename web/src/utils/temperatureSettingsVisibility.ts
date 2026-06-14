import type { RouterConfig } from "../api/types";

/** Toggle visibility of temperature-related settings rows. */
export function applyTemperatureFieldVisibility(
  els: { probeLabel: HTMLElement; triacCap: HTMLElement },
  present: boolean,
): void {
  els.probeLabel.hidden = !present;
  els.triacCap.hidden = !present;
}

/** Identity-card autosave patch; omits label when probe absent. */
export function collectIdentityTemperaturePatch(
  present: boolean,
  temperatureLabel: string,
): Partial<Pick<RouterConfig, "temperature_label">> {
  if (!present) return {};
  return { temperature_label: temperatureLabel };
}

/** Advanced-sources autosave patch; omits cap when probe absent. */
export function collectAdvancedTemperaturePatch(
  present: boolean,
  triacOverrideMaxTempC: number,
): Partial<Pick<RouterConfig, "triac_override_max_temp_c">> {
  if (!present) return {};
  return { triac_override_max_temp_c: triacOverrideMaxTempC };
}
