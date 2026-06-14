import { describe, expect, it, beforeEach } from "vitest";
import { deviceInfo } from "../state/store";
import { productModePioEnv } from "../state/productMode";
import {
  compiledMeterWires,
  hasCompiledMeter,
  hasMultiAction,
  hasSurplusRegulation,
  hasTriac,
  isSingleMeterProfile,
  meterPackId,
  showAdvancedMeterSourcesSection,
} from "./firmwareCaps";

describe("firmwareCaps", () => {
  beforeEach(() => {
    deviceInfo.set(undefined);
    productModePioEnv.set(null);
  });

  it("router mode implies triac even when triac key omitted", () => {
    deviceInfo.set({
      router_name: "x",
      firmware_version: "1",
      probe_house_name: "",
      probe_second_name: "",
      temperature_label: "",
      capabilities: {
        product_profile: "jsy_mk194_router",
        firmware_capabilities: { surplus_regulation: true },
      },
    });
    expect(hasSurplusRegulation()).toBe(true);
    expect(hasTriac()).toBe(true);
  });

  it("strict surplus_regulation", () => {
    deviceInfo.set({
      router_name: "x",
      firmware_version: "1",
      probe_house_name: "",
      probe_second_name: "",
      temperature_label: "",
      capabilities: {
        product_profile: "jsy_mk194_meter",
        firmware_capabilities: { surplus_regulation: false, triac: false },
      },
    });
    expect(hasSurplusRegulation()).toBe(false);
    expect(hasTriac()).toBe(false);
  });

  it("meter pack single profile", () => {
    deviceInfo.set({
      router_name: "x",
      firmware_version: "1",
      probe_house_name: "",
      probe_second_name: "",
      temperature_label: "",
      capabilities: {
        product_profile: "jsy_mk194_meter",
        firmware_capabilities: {
          meter_pack: "jsy_mk194",
          meters: ["JsyMk194"],
        },
      },
    });
    expect(isSingleMeterProfile()).toBe(true);
    expect(compiledMeterWires()).toEqual(["JsyMk194"]);
    expect(hasCompiledMeter("Linky")).toBe(false);
    expect(meterPackId()).toBe("jsy_mk194");
  });

  it("full router before caps load shows all meters", () => {
    expect(hasCompiledMeter("Linky")).toBe(true);
    expect(hasMultiAction()).toBe(false);
  });

  it("showAdvancedMeterSourcesSection false for single-meter JSY", () => {
    deviceInfo.set({
      router_name: "x",
      firmware_version: "1",
      probe_house_name: "",
      probe_second_name: "",
      temperature_label: "",
      capabilities: {
        product_profile: "jsy_mk194_router",
        firmware_capabilities: { meters: ["JsyMk194"], surplus_regulation: true },
      },
    });
    expect(showAdvancedMeterSourcesSection()).toBe(false);
  });

  it("showAdvancedMeterSourcesSection true when Pmqtt compiled", () => {
    deviceInfo.set({
      router_name: "x",
      firmware_version: "1",
      probe_house_name: "",
      probe_second_name: "",
      temperature_label: "",
      capabilities: {
        product_profile: "full_router",
        firmware_capabilities: { meters: ["JsyMk194", "Pmqtt"], surplus_regulation: true },
      },
    });
    expect(showAdvancedMeterSourcesSection()).toBe(true);
  });

  it("showAdvancedMeterSourcesSection false when only Linky among multi-meter", () => {
    deviceInfo.set({
      router_name: "x",
      firmware_version: "1",
      probe_house_name: "",
      probe_second_name: "",
      temperature_label: "",
      capabilities: {
        product_profile: "linky_router",
        firmware_capabilities: { meters: ["JsyMk194", "Linky"], surplus_regulation: true },
      },
    });
    expect(showAdvancedMeterSourcesSection()).toBe(false);
  });

  it("showAdvancedMeterSourcesSection true for non-Linky multi-meter without Pmqtt", () => {
    deviceInfo.set({
      router_name: "x",
      firmware_version: "1",
      probe_house_name: "",
      probe_second_name: "",
      temperature_label: "",
      capabilities: {
        product_profile: "jsy_mk333_router",
        firmware_capabilities: { meters: ["JsyMk194", "JsyMk333"], surplus_regulation: true },
      },
    });
    expect(showAdvancedMeterSourcesSection()).toBe(true);
  });
});
