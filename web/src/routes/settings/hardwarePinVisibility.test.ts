import { describe, expect, it, beforeEach } from "vitest";
import { deviceInfo } from "../../state/store";
import { isEditorPinVisible, isSummaryRowVisible } from "./hardwarePinVisibility";
import { HARDWARE_PIN_SUMMARY_ROWS } from "./hardwarePinMeta";

describe("hardwarePinVisibility", () => {
  beforeEach(() => {
    deviceInfo.set(undefined);
  });

  it("shows triac pins when surplus_regulation true without triac key", () => {
    deviceInfo.set({
      router_name: "x",
      firmware_version: "1",
      probe_house_name: "",
      probe_second_name: "",
      temperature_label: "",
      capabilities: {
        product_profile: "jsy_mk194_router",
        firmware_capabilities: { surplus_regulation: true, meters: ["JsyMk194"] },
      },
    });
    expect(isEditorPinVisible("pin_triac_dim", "JsyMk194")).toBe(true);
    expect(isEditorPinVisible("pin_zero_cross", "JsyMk194")).toBe(true);
    expect(isEditorPinVisible("pin_uart_rx", "JsyMk194")).toBe(true);
  });

  it("hides triac pins on meter-gateway when caps loaded", () => {
    deviceInfo.set({
      router_name: "x",
      firmware_version: "1",
      probe_house_name: "",
      probe_second_name: "",
      temperature_label: "",
      capabilities: {
        product_profile: "jsy_mk194_meter",
        firmware_capabilities: { surplus_regulation: false, triac: false, meters: ["JsyMk194"] },
      },
    });
    expect(isEditorPinVisible("pin_triac_dim", "JsyMk194")).toBe(false);
    expect(isEditorPinVisible("pin_zero_cross", "JsyMk194")).toBe(false);
    expect(isEditorPinVisible("pin_uart_rx", "JsyMk194")).toBe(true);
  });

  it("hides uncompiled meter pins when caps loaded", () => {
    deviceInfo.set({
      router_name: "x",
      firmware_version: "1",
      probe_house_name: "",
      probe_second_name: "",
      temperature_label: "",
      capabilities: {
        firmware_capabilities: { meters: ["JsyMk194"], surplus_regulation: true },
      },
    });
    const uart = HARDWARE_PIN_SUMMARY_ROWS.find((r) => r.kind === "uart")!;
    expect(isSummaryRowVisible(uart, "JsyMk194")).toBe(true);
    expect(isSummaryRowVisible(uart, "Linky")).toBe(false);
    expect(isEditorPinVisible("pin_uart_rx", "Linky")).toBe(false);
  });
});
