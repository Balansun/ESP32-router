import { describe, expect, it } from "vitest";
import type { ActionConfig } from "../../api/types";
import {
  actionSupportsDailyCapWh,
  blankHttpAction,
  ensureNormalised,
  MAX_ACTIONS,
  normalizeDailyCapWh,
  normaliseForApi,
  SCHEMA_VERSION,
} from "./model";
import { MODE_INACTIF } from "./regulationMode";

describe("actions model", () => {
  it("ensureNormalised returns default triac when empty", () => {
    const a = ensureNormalised([]);
    expect(a).toHaveLength(1);
    expect(a[0].kind).toBe("triac");
    expect(a[0].triac_sensitivity).toBe(50);
  });

  it("normaliseForApi forces triac on index 0", () => {
    const list = ensureNormalised([]);
    const out = normaliseForApi({ ...list[0], host: "x", kind: "remote_http" }, 0);
    expect(out.kind).toBe("triac");
    expect(out.triac_sensitivity).toBe(50);
  });

  it("normaliseForApi maps localhost to local_gpio", () => {
    const a = blankHttpAction(1);
    a.host = "localhost";
    const out = normaliseForApi(a, 1);
    expect(out.kind).toBe("local_gpio");
  });

  it("exports schema constants", () => {
    expect(SCHEMA_VERSION).toBe(2);
    expect(MAX_ACTIONS).toBe(20);
  });

  it("actionSupportsDailyCapWh only for triac index 0", () => {
    expect(actionSupportsDailyCapWh(0)).toBe(true);
    expect(actionSupportsDailyCapWh(1)).toBe(false);
  });

  it("normalizeDailyCapWh pads to three slots", () => {
    expect(normalizeDailyCapWh()).toEqual([0, 0, 0]);
    expect(normalizeDailyCapWh([100, 200])).toEqual([100, 200, 0]);
    expect(normalizeDailyCapWh([1, 2, 3, 4])).toEqual([1, 2, 3]);
  });

  it("ensureNormalised preserves indices and copies periods", () => {
    const raw = [
      {
        index: 0,
        kind: "triac" as const,
        title: "T",
        regulation_mode: 1,
        triac_sensitivity: 40,
        repeat_sec: 0,
        tempo_sec: 0,
        periods: [{ mode: "power" as const, hour_end: 2400, power_min_w: 0, power_max_w: 50, temp_inf_c: 150, temp_sup_c: 150 }],
      },
    ];
    const out = ensureNormalised(raw);
    expect(out[0].index).toBe(0);
    expect(out[0].triac_sensitivity).toBe(40);
    expect(out[0].periods).not.toBe(raw[0].periods);
  });

  it("ensureNormalised keeps default port 80 for remote actions", () => {
    const out = ensureNormalised([
      {
        index: 0,
        kind: "triac" as const,
        title: "T",
        regulation_mode: 1,
        triac_sensitivity: 50,
        repeat_sec: 0,
        tempo_sec: 0,
        periods: [],
      },
      {
        index: 1,
        kind: "remote_http" as const,
        title: "R",
        regulation_mode: 1,
        repeat_sec: 0,
        tempo_sec: 0,
        periods: [],
      },
    ]);
    expect(out[1].port).toBe(80);
    expect(out[1].triac_sensitivity).toBeUndefined();
  });

  it("ensureNormalised uses port when triac_sensitivity missing", () => {
    const raw = [
      {
        index: 0,
        kind: "triac" as const,
        title: "T",
        regulation_mode: 1,
        port: 33,
        repeat_sec: 0,
        tempo_sec: 0,
        periods: [],
      },
    ];
    const out = ensureNormalised(raw);
    expect(out[0].triac_sensitivity).toBe(33);
    expect(out[0].port).toBe(33);
  });

  it("normaliseForApi keeps remote_http for non-localhost hosts", () => {
    const a = blankHttpAction(2);
    a.host = "192.168.1.20";
    const out = normaliseForApi(a, 2);
    expect(out.kind).toBe("remote_http");
    expect(out.host).toBe("192.168.1.20");
  });

  it("normaliseForApi defaults missing regulation_mode and empty host on remote", () => {
    const a = blankHttpAction(1);
    Reflect.deleteProperty(a, "regulation_mode");
    Reflect.deleteProperty(a, "host");
    const out = normaliseForApi(a, 1);
    expect(out.regulation_mode).toBe(MODE_INACTIF);
    expect(out.host).toBe("");
  });

  it("normaliseForApi uses default port 80 when port is 0", () => {
    const a = blankHttpAction(4);
    a.port = 0;
    const out = normaliseForApi(a, 4);
    expect(out.port).toBe(80);
  });

  it("ensureNormalised replaces missing periods with factory triac default", () => {
    const out = ensureNormalised([
      {
        index: 0,
        kind: "triac",
        title: "T",
        regulation_mode: 1,
        triac_sensitivity: 50,
        repeat_sec: 0,
        tempo_sec: 0,
        periods: undefined as unknown as ActionConfig["periods"],
      },
    ]);
    expect(out[0].periods).toHaveLength(1);
    expect(out[0].periods[0].hour_end).toBe(2400);
    expect(out[0].periods[0].mode).toBe("power");
  });

  it("ensureNormalised drops corrupt period garbage from API", () => {
    const out = ensureNormalised([
      {
        index: 0,
        kind: "triac",
        title: "T",
        regulation_mode: 1,
        triac_sensitivity: 50,
        repeat_sec: 0,
        tempo_sec: 0,
        periods: Array.from({ length: 50 }, () => ({
          mode: "power" as const,
          hour_end: -1515870811,
          power_min_w: -1515870811,
          power_max_w: 0,
          temp_inf_c: 150,
          temp_sup_c: 150,
        })),
      },
    ]);
    expect(out[0].periods.length).toBeLessThanOrEqual(8);
    expect(out[0].periods[0].hour_end).toBe(2400);
  });

  it("normaliseForApi defaults triac sensitivity when port and sensitivity missing", () => {
    const raw = ensureNormalised([])[0];
    Reflect.deleteProperty(raw, "triac_sensitivity");
    Reflect.deleteProperty(raw, "port");
    expect(normaliseForApi(raw, 0).triac_sensitivity).toBe(50);
  });
});
