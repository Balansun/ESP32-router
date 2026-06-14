import { isProbeTemperatureReading } from "./format";

export type LegionellaBannerState = "hidden" | "sensor_present";

export function resolveLegionellaBannerState(opts: {
  gatingDisabled: boolean;
  temperatureC: number;
}): LegionellaBannerState {
  if (!opts.gatingDisabled) return "hidden";
  if (!isProbeTemperatureReading(opts.temperatureC)) return "hidden";
  return "sensor_present";
}
