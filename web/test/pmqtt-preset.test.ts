import { describe, expect, it } from "vitest";
import {
  detectPmqttPreset,
  pmqttSchemaForPreset,
} from "../src/components/PmqttPresetFields";

describe("Pmqtt presets", () => {
  it("maps simple_pw preset to Pw,Pf schema", () => {
    expect(pmqttSchemaForPreset("simple_pw", "")).toBe("Pw,Pf");
    expect(detectPmqttPreset("Pw,Pf")).toBe("simple_pw");
  });

  it("maps house snapshot preset", () => {
    expect(pmqttSchemaForPreset("house_snapshot", "")).toBe("house");
    expect(detectPmqttPreset("house")).toBe("house_snapshot");
  });

  it("custom preset uses user string", () => {
    expect(pmqttSchemaForPreset("custom", "Pw,Pf,Pv")).toBe("Pw,Pf,Pv");
    expect(pmqttSchemaForPreset("custom", "   ")).toBe("Pw");
    expect(detectPmqttPreset("other")).toBe("custom");
    expect(detectPmqttPreset("Pw")).toBe("simple_pw");
  });
});
