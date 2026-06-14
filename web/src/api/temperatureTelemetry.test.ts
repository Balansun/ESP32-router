import { describe, expect, it, vi, beforeEach } from "vitest";
import {
  fetchLiveTemperatureTelemetry,
  historySeriesHasProbeTemp,
  resolveLiveTemperatureC,
  temperatureSensorAvailable,
} from "./temperatureTelemetry";

const getMeasurements = vi.fn();
const getHealth = vi.fn();

vi.mock("./client", () => ({
  api: {
    getMeasurements: (...args: unknown[]) => getMeasurements(...args),
    getHealth: (...args: unknown[]) => getHealth(...args),
  },
}));

describe("fetchLiveTemperatureTelemetry", () => {
  beforeEach(() => {
    getMeasurements.mockReset();
    getHealth.mockReset();
  });

  it("uses measurements when available", async () => {
    getMeasurements.mockResolvedValue({
      temperature_c: 22.1,
      temperature_sensors: { gpio: 13, max_count: 2, discovered_count: 1, bus_active: true, sensors: [] },
    });
    const live = await fetchLiveTemperatureTelemetry();
    expect(live.temperature_c).toBe(22.1);
    expect(live.telemetry_ready).toBe(true);
    expect(getHealth).not.toHaveBeenCalled();
  });

  it("falls back to health when measurements is not ready", async () => {
    getMeasurements.mockRejectedValue(new Error("503"));
    getHealth.mockResolvedValue({
      ok: true,
      telemetry_ready: false,
      temperature_c: 19.5,
      temperature_sensors: { gpio: 13, max_count: 2, discovered_count: 1, bus_active: true, sensors: [] },
    });
    const live = await fetchLiveTemperatureTelemetry();
    expect(live.temperature_c).toBe(19.5);
    expect(live.telemetry_ready).toBe(false);
    expect(getHealth).toHaveBeenCalled();
  });
});

describe("resolveLiveTemperatureC", () => {
  it("prefers aggregate temperature_c", () => {
    expect(resolveLiveTemperatureC({ temperature_c: 21.5 })).toBe(21.5);
  });

  it("falls back to primary valid slot", () => {
    expect(
      resolveLiveTemperatureC({
        temperature_sensors: {
          gpio: 13,
          max_count: 2,
          discovered_count: 1,
          bus_active: true,
          sensors: [
            { slot: 0, enabled: true, valid: true, primary: true, temperature_c: 18.2 },
          ],
        },
      }),
    ).toBe(18.2);
  });

  it("rejects sentinel readings", () => {
    expect(resolveLiveTemperatureC({ temperature_c: -127 })).toBeUndefined();
  });
});

describe("temperatureSensorAvailable", () => {
  it("is true when bus reports discovered sensors", () => {
    expect(
      temperatureSensorAvailable({
        temperature_sensors: {
          gpio: 13,
          max_count: 2,
          discovered_count: 1,
          bus_active: true,
          sensors: [],
        },
      }),
    ).toBe(true);
  });
});

describe("historySeriesHasProbeTemp", () => {
  it("detects non-flat valid samples", () => {
    expect(historySeriesHasProbeTemp([0, 22, 0])).toBe(true);
    expect(historySeriesHasProbeTemp([0, 0, 0])).toBe(false);
    expect(historySeriesHasProbeTemp([-127])).toBe(false);
  });
});
