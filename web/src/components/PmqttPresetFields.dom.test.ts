import { describe, expect, it, vi } from "vitest";
import { en } from "../i18n/locales/en";
import { buildPmqttPresetFields } from "./PmqttPresetFields";

describe("PmqttPresetFields DOM", () => {
  it("buildPmqttPresetFields switches presets", () => {
    const onChange = vi.fn();
    const ui = buildPmqttPresetFields(en, "Pw,Pf", onChange);
    document.body.append(...ui.sectionRows);
    expect(ui.getSchema()).toBe("Pw,Pf");
    const select = document.getElementById("pmqtt_preset") as HTMLSelectElement;
    select.value = "simple_pw";
    select.dispatchEvent(new Event("change", { bubbles: true }));
    expect(ui.getSchema()).toBe("Pw,Pf");
    select.value = "house_snapshot";
    select.dispatchEvent(new Event("change", { bubbles: true }));
    expect(ui.getSchema()).toBe("house");
    select.value = "custom";
    select.dispatchEvent(new Event("change", { bubbles: true }));
    const custom = document.getElementById(
      "pmqtt_schema_custom",
    ) as HTMLInputElement;
    custom.value = "Pw,Pf,Pv";
    custom.dispatchEvent(new Event("input", { bubbles: true }));
    expect(ui.getSchema()).toBe("Pw,Pf,Pv");
    expect(onChange).toHaveBeenCalled();
  });

  it("initial house_snapshot preset sets hint", () => {
    const ui = buildPmqttPresetFields(en, "house", () => {});
    document.body.append(...ui.sectionRows);
    expect(ui.getSchema()).toBe("house");
  });

  it("initial custom preset shows custom hint", () => {
    const ui = buildPmqttPresetFields(en, "Pw,Pf,Pv", () => {});
    document.body.append(...ui.sectionRows);
    expect(ui.getSchema()).toBe("Pw,Pf,Pv");
  });
});
