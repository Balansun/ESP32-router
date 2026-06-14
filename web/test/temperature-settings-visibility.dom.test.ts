import { describe, expect, it } from "vitest";
import {
  applyTemperatureFieldVisibility,
  collectAdvancedTemperaturePatch,
  collectIdentityTemperaturePatch,
} from "../src/utils/temperatureSettingsVisibility";

describe("collectIdentityTemperaturePatch", () => {
  it("includes temperature_label when probe present", () => {
    expect(collectIdentityTemperaturePatch(true, "Tank")).toEqual({
      temperature_label: "Tank",
    });
  });

  it("omits temperature_label when probe absent", () => {
    expect(collectIdentityTemperaturePatch(false, "Tank")).toEqual({});
  });
});

describe("collectAdvancedTemperaturePatch", () => {
  it("includes triac cap when probe present", () => {
    expect(collectAdvancedTemperaturePatch(true, 70)).toEqual({
      triac_override_max_temp_c: 70,
    });
  });

  it("omits triac cap when probe absent", () => {
    expect(collectAdvancedTemperaturePatch(false, 70)).toEqual({});
  });
});

describe("applyTemperatureFieldVisibility", () => {
  it("shows fields when probe present", () => {
    const probeLabel = document.createElement("div");
    const triacCap = document.createElement("div");
    applyTemperatureFieldVisibility({ probeLabel, triacCap }, true);
    expect(probeLabel.hidden).toBe(false);
    expect(triacCap.hidden).toBe(false);
  });

  it("hides fields when probe absent", () => {
    const probeLabel = document.createElement("div");
    const triacCap = document.createElement("div");
    applyTemperatureFieldVisibility({ probeLabel, triacCap }, false);
    expect(probeLabel.hidden).toBe(true);
    expect(triacCap.hidden).toBe(true);
  });
});
