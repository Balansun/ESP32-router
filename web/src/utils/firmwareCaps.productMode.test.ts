import { beforeEach, describe, expect, it } from "vitest";
import { deviceInfo } from "../state/store";
import { productModePioEnv } from "../state/productMode";
import {
  hasCompiledMeter,
  hasMultiAction,
  hasSurplusRegulation,
  isSingleMeterProfile,
  showActionsNav,
  showAdvancedMeterSourcesSection,
} from "./firmwareCaps";

function setFullDevice() {
  deviceInfo.set({
    router_name: "Balansun",
    firmware_version: "0.1.0",
    probe_house_name: "",
    probe_second_name: "",
    temperature_label: "",
    capabilities: {
      product_profile: "full_router",
      firmware_capabilities: {
        surplus_regulation: true,
        multi_action: true,
        meter_pack: "full",
        meters: ["JsyMk194", "Linky", "Pmqtt"],
      },
    },
  });
}

describe("firmwareCaps product mode overlay", () => {
  beforeEach(() => {
    deviceInfo.set(undefined);
    productModePioEnv.set(null);
    localStorage.removeItem("balansun_product_mode_pio_env");
  });

  it("full firmware defaults to full router caps", () => {
    setFullDevice();
    productModePioEnv.set("wroom32");
    expect(hasSurplusRegulation()).toBe(true);
    expect(hasMultiAction()).toBe(true);
    expect(isSingleMeterProfile()).toBe(false);
    expect(showAdvancedMeterSourcesSection()).toBe(true);
  });

  it("meter gateway mode hides routing features", () => {
    setFullDevice();
    productModePioEnv.set("linky_meter");
    expect(hasSurplusRegulation()).toBe(false);
    expect(showActionsNav()).toBe(false);
    expect(isSingleMeterProfile()).toBe(true);
    expect(hasCompiledMeter("Linky")).toBe(true);
    expect(hasCompiledMeter("Pmqtt")).toBe(false);
  });

  it("stripped firmware ignores product mode overlay", () => {
    deviceInfo.set({
      router_name: "x",
      firmware_version: "1",
      probe_house_name: "",
      probe_second_name: "",
      temperature_label: "",
      capabilities: {
        product_profile: "jsy_mk194_router",
        firmware_capabilities: {
          surplus_regulation: true,
          meters: ["JsyMk194"],
        },
      },
    });
    productModePioEnv.set("linky_meter");
    expect(hasSurplusRegulation()).toBe(true);
    expect(hasCompiledMeter("JsyMk194")).toBe(true);
  });
});
