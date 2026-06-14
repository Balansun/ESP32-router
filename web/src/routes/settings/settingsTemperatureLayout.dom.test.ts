import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import type { Measurements, RouterConfig } from "../../api/types";
import { buildTemperatureSettingsSection } from "./settingsTemperatureLayout";

const apiMock = vi.hoisted(() => ({
  getMeasurements: vi.fn(),
  patchConfig: vi.fn(),
}));

vi.mock("../../api/client", () => ({ api: apiMock }));
vi.mock("../../components/Toast", () => ({ toast: vi.fn() }));

const baseCfg = (): RouterConfig =>
  ({
    temperature_label: "Tank",
    temp_gpio: 21,
    temperature_slots: [
      { enabled: true, label: "Ballon", address: "" },
      { enabled: true, label: "Ambient", address: "28AABBCCDDEEFF00" },
    ],
  }) as RouterConfig;

const sampleMeasurements = (): Measurements =>
  ({
    date_valid: true,
    date: "2026-06-01T12:00:00",
    source: "JsyMk194",
    temperature_c: 48.2,
    temperature_sensors: {
      gpio: 21,
      max_count: 2,
      discovered_count: 2,
      bus_active: true,
      sensors: [
        {
          slot: 0,
          enabled: true,
          label: "Ballon",
          address: "28AABBCCDDEEFF00",
          temperature_c: 48.2,
          valid: true,
          primary: true,
        },
        {
          slot: 1,
          enabled: true,
          label: "Ambient",
          address: "28BBCCDDEEFF0011",
          temperature_c: 19.1,
          valid: true,
          primary: false,
        },
      ],
    },
    house: {} as Measurements["house"],
    raw_meter: {} as Measurements["raw_meter"],
  }) as Measurements;

describe("buildTemperatureSettingsSection", () => {
  beforeEach(() => {
    vi.useFakeTimers();
    vi.clearAllMocks();
    apiMock.getMeasurements.mockResolvedValue(sampleMeasurements());
    apiMock.patchConfig.mockResolvedValue(undefined);
  });

  afterEach(() => {
    vi.useRealTimers();
  });

  it("builds GPIO select from config and legacy label fallback", () => {
    const section = buildTemperatureSettingsSection({
      temperature_label: "Legacy",
    } as RouterConfig);
    const select = section.root.querySelector("#temp_gpio") as HTMLSelectElement;
    expect(select?.value).toBe("13");
    expect(section.root.textContent).toMatch(/Legacy|Slot 1|emplacement 1|slot 1/i);
    const cleanup = (section.root as HTMLElement & { __tempPollCleanup?: () => void })
      .__tempPollCleanup;
    cleanup?.();
  });

  it("refreshLive shows discovered count and primary slot", async () => {
    const section = buildTemperatureSettingsSection(baseCfg());
    await vi.runOnlyPendingTimersAsync();
    section.refreshLive(sampleMeasurements());
    expect(section.root.textContent).toMatch(/48/);
    expect(section.root.textContent).toMatch(/19/);
    expect(section.root.textContent).toMatch(/primary|primaire/i);
    (section.root as HTMLElement & { __tempPollCleanup?: () => void }).__tempPollCleanup?.();
  });

  it("refreshLive handles missing telemetry", () => {
    const section = buildTemperatureSettingsSection(baseCfg());
    section.refreshLive(undefined);
    expect(section.root.textContent).toMatch(/unavailable|indisponible/i);
    (section.root as HTMLElement & { __tempPollCleanup?: () => void }).__tempPollCleanup?.();
  });

  it("refreshLive shows invalid reading and ROM address", () => {
    const section = buildTemperatureSettingsSection(baseCfg());
    const m = sampleMeasurements();
    if (m.temperature_sensors?.sensors?.[1]) {
      m.temperature_sensors.sensors[1].valid = false;
      m.temperature_sensors.sensors[1].temperature_c = null;
    }
    section.refreshLive(m);
    expect(section.root.textContent).toMatch(/no valid|pas de lecture/i);
    section.refreshLive(sampleMeasurements());
    expect(section.root.textContent).toMatch(/28BBCCDDEEFF0011/i);
    (section.root as HTMLElement & { __tempPollCleanup?: () => void }).__tempPollCleanup?.();
  });

  it("refreshLive treats missing slot readings as disabled", () => {
    const section = buildTemperatureSettingsSection(baseCfg());
    const m = sampleMeasurements();
    if (m.temperature_sensors) {
      m.temperature_sensors.sensors = m.temperature_sensors.sensors?.filter((s) => s.slot === 0);
    }
    section.refreshLive(m);
    expect(section.root.textContent).toMatch(/disabled|désactivée/i);
    (section.root as HTMLElement & { __tempPollCleanup?: () => void }).__tempPollCleanup?.();
  });

  it("refreshLive marks disabled slots", () => {
    const section = buildTemperatureSettingsSection(baseCfg());
    const m = sampleMeasurements();
    if (m.temperature_sensors?.sensors?.[1]) {
      m.temperature_sensors.sensors[1].enabled = false;
    }
    section.refreshLive(m);
    expect(section.root.textContent).toMatch(/disabled|désactivée/i);
    (section.root as HTMLElement & { __tempPollCleanup?: () => void }).__tempPollCleanup?.();
  });

  it("collectPatch returns gpio and slots", () => {
    const section = buildTemperatureSettingsSection(baseCfg());
    const patch = section.collectPatch();
    expect(patch.temp_gpio).toBe(21);
    expect(patch.temperature_slots).toHaveLength(2);
    expect(patch.temperature_label).toBe("Ballon");
    (section.root as HTMLElement & { __tempPollCleanup?: () => void }).__tempPollCleanup?.();
  });

  it("defaultSlots uses legacy label when API slots are absent", () => {
    const section = buildTemperatureSettingsSection({
      temperature_label: "Legacy probe",
    } as RouterConfig);
    const labelInput = section.root.querySelector(
      "#temperature_slot_0_label",
    ) as HTMLInputElement;
    expect(labelInput?.value).toBe("Legacy probe");
    (section.root as HTMLElement & { __tempPollCleanup?: () => void }).__tempPollCleanup?.();
  });

  it("refreshLive hides primary hint when no slot is primary", () => {
    const section = buildTemperatureSettingsSection(baseCfg());
    const m = sampleMeasurements();
    for (const s of m.temperature_sensors?.sensors ?? []) {
      s.primary = false;
    }
    section.refreshLive(m);
    expect(section.root.querySelector("[hidden]")?.textContent ?? "").not.toMatch(/primary|primaire/i);
    (section.root as HTMLElement & { __tempPollCleanup?: () => void }).__tempPollCleanup?.();
  });

  it("poll interval ignores measurement errors", async () => {
    apiMock.getMeasurements.mockRejectedValue(new Error("offline"));
    const section = buildTemperatureSettingsSection(baseCfg());
    await vi.advanceTimersByTimeAsync(5000);
    expect(apiMock.getMeasurements).toHaveBeenCalled();
    (section.root as HTMLElement & { __tempPollCleanup?: () => void }).__tempPollCleanup?.();
  });

  it("attachAutosave persists temperature config", async () => {
    const section = buildTemperatureSettingsSection(baseCfg());
    const card = section.root.querySelector(".card") as HTMLElement;
    const ctrl = new AbortController();
    const onSaved = vi.fn();
    const autosave = section.attachAutosave(
      {
        card,
        signal: ctrl.signal,
        labels: { pending: "p", saving: "s", saved: "ok", error: "e" },
      },
      onSaved,
    );
    const labelInput = section.root.querySelector(
      "#temperature_slot_0_label",
    ) as HTMLInputElement;
    labelInput.value = "Updated";
    labelInput.dispatchEvent(new Event("input", { bubbles: true }));
    await autosave.flush();
    expect(apiMock.patchConfig).toHaveBeenCalled();
    expect(onSaved).toHaveBeenCalled();
    (section.root as HTMLElement & { __tempPollCleanup?: () => void }).__tempPollCleanup?.();
  });

  it("covers defaultSlots fallbacks and collectPatch nullish fallbacks", async () => {
    const cfgMissingLabel: RouterConfig = {
      temperature_label: "",
      temp_gpio: 21,
      // No temperature_slots -> triggers `defaultSlots()` fallback label branch.
    } as RouterConfig;

    const section1 = buildTemperatureSettingsSection(cfgMissingLabel);

    const m1 = sampleMeasurements();
    if (m1.temperature_sensors) {
      // Cover `??` fallbacks in `refreshLive()` line chain.
      (m1.temperature_sensors as any).discovered_count = undefined;
      (m1.temperature_sensors as any).max_count = undefined;
      (m1.temperature_sensors as any).gpio = undefined;
      // Cover `ts.sensors ?? []` branch.
      (m1.temperature_sensors as any).sensors = undefined;
    }
    section1.refreshLive(m1);

    const m2 = sampleMeasurements();
    if (m2.temperature_sensors?.sensors) {
      // Ensure at least one enabled slot has a truthy `address` to cover ROM label branch.
      m2.temperature_sensors.sensors = m2.temperature_sensors.sensors.map((s) => {
        if (s.slot === 0) {
          return {
            ...s,
            enabled: true,
            address: "28AABBCCDDEEFF00",
            valid: true,
            temperature_c: 48.2,
          };
        }
        return s;
      });
      m2.temperature_sensors.discovered_count = 0;
      m2.temperature_sensors.max_count = 2;
      m2.temperature_sensors.gpio = 21;
    }
    section1.refreshLive(m2);
    (section1.root as HTMLElement & { __tempPollCleanup?: () => void }).__tempPollCleanup?.();

    const cfgCollect = {
      temperature_label: "FallbackLabel",
      temp_gpio: 21,
      temperature_slots: [
        // Intentionally omit `address` to cover `slots[i]?.address ?? ""`.
        { enabled: true, label: "Slot0" },
        { enabled: false, label: "Slot1" },
      ],
    } as any as RouterConfig;

    const section2 = buildTemperatureSettingsSection(cfgCollect);
    const gpioSelect = section2.root.querySelector("#temp_gpio") as HTMLSelectElement;
    gpioSelect.value = "0";
    const labelInput0 = section2.root.querySelector("#temperature_slot_0_label") as HTMLInputElement;
    labelInput0.value = "";

    const patch = section2.collectPatch();
    expect(patch.temp_gpio).toBe(13);
    expect(patch.temperature_slots?.[0]?.address).toBe("");
    expect(patch.temperature_slots?.[1]?.address).toBe("");
    expect(patch.temperature_label).toBe("FallbackLabel");

    (section2.root as HTMLElement & { __tempPollCleanup?: () => void }).__tempPollCleanup?.();
  });

  it("covers nullish slot enabled/label fallbacks", async () => {
    const cfg = {
      temperature_label: "Tank",
      temp_gpio: 21,
      temperature_slots: [
        { enabled: true, label: "Ballon", address: "" },
        // Missing `enabled` + `label` should trigger `?? false` and `?? ""` branches.
        { enabled: undefined, label: undefined, address: "" },
      ],
    } as any as RouterConfig;

    const section = buildTemperatureSettingsSection(cfg);
    const slot1Label = section.root.querySelector("#temperature_slot_1_label") as HTMLInputElement | null;
    expect(slot1Label?.value ?? "").toBe("");
    const cleanup = (section.root as HTMLElement & { __tempPollCleanup?: () => void }).__tempPollCleanup;
    cleanup?.();
  });
});
