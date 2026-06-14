import { api } from "./client";
import type { Measurements, TemperatureSensorsTelemetry } from "./types";
import { isProbeTemperatureReading } from "../utils/format";

type TelemetryFetchOpts = { signal?: AbortSignal };

export interface LiveTemperatureTelemetry {
  temperature_c?: number;
  temperature_sensors?: TemperatureSensorsTelemetry;
  telemetry_ready?: boolean;
}

/** Measurements when the meter is up; otherwise /health (bench without meter/triac). */
export async function fetchLiveTemperatureTelemetry(
  opts?: TelemetryFetchOpts,
): Promise<LiveTemperatureTelemetry> {
  try {
    const m = await api.getMeasurements(opts);
    return {
      temperature_c: m.temperature_c,
      temperature_sensors: m.temperature_sensors,
      telemetry_ready: true,
    };
  } catch {
    const health = await api.getHealth(opts).catch(() => null);
    if (!health) return {};
    const h = health as typeof health & LiveTemperatureTelemetry;
    return {
      temperature_c: h.temperature_c,
      temperature_sensors: h.temperature_sensors,
      telemetry_ready: health.telemetry_ready,
    };
  }
}

export function measurementsWithTemperatureTelemetry(
  live: LiveTemperatureTelemetry,
): Pick<Measurements, "temperature_c" | "temperature_sensors"> {
  return {
    temperature_c: live.temperature_c,
    temperature_sensors: live.temperature_sensors,
  };
}

/** Primary °C from aggregate field or first valid DS18B20 slot reading. */
export function resolveLiveTemperatureC(
  live: Pick<LiveTemperatureTelemetry, "temperature_c" | "temperature_sensors">,
): number | undefined {
  if (isProbeTemperatureReading(live.temperature_c)) return live.temperature_c;
  const sensors = live.temperature_sensors?.sensors;
  if (!sensors?.length) return undefined;
  const primary = sensors.find((s) => s.primary && s.valid);
  const firstValid = sensors.find((s) => s.valid);
  const reading = primary ?? firstValid;
  if (reading && isProbeTemperatureReading(reading.temperature_c)) {
    return reading.temperature_c as number;
  }
  return undefined;
}

/** True when a probe is configured/discovered or a live reading exists. */
export function temperatureSensorAvailable(
  live: Pick<LiveTemperatureTelemetry, "temperature_c" | "temperature_sensors">,
): boolean {
  if (resolveLiveTemperatureC(live) != null) return true;
  const meta = live.temperature_sensors;
  if (!meta) return false;
  if (meta.bus_active && meta.discovered_count > 0) return true;
  return !!meta.sensors?.some((s) => s.enabled && s.valid);
}

/** True when a downsampled history ring has at least one plausible probe sample. */
export function historySeriesHasProbeTemp(series: number[]): boolean {
  const valid = series.filter((v) => isProbeTemperatureReading(v));
  if (valid.length === 0) return false;
  if (valid.every((v) => v === 0)) return false;
  return true;
}
