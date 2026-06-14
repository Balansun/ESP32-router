import { describe, expect, it, beforeEach } from "vitest";
import { deviceInfo } from "../../state/store";
import {
  isMeterCompiledForSource,
  isMeterPinActiveForSource,
  isSummaryRowActiveForSource,
  meterGpioGroupsForSource,
  resolveSourceWireId,
  shouldShowMeterPinFilterHint,
  uartPresenceIdForSource,
} from "./meterPinRegistry";
import { HARDWARE_PIN_SUMMARY_ROWS } from "./hardwarePinMeta";

describe("meterPinRegistry", () => {
  beforeEach(() => {
    deviceInfo.set(undefined);
  });

  it("resolves canonical firmware wire ids only", () => {
    expect(resolveSourceWireId("JsyMk194")).toBe("JsyMk194");
    expect(resolveSourceWireId("jsymk194")).toBe("JsyMk194");
    expect(resolveSourceWireId("Linky")).toBe("Linky");
    expect(resolveSourceWireId("BalansunPeer")).toBe("BalansunPeer");
    expect(resolveSourceWireId("UxIx2")).toBeUndefined();
    expect(resolveSourceWireId("uxix2")).toBeUndefined();
  });

  it("declares GPIO groups per meter source", () => {
    expect(meterGpioGroupsForSource("JsyMk194")).toEqual(["uart"]);
    expect(meterGpioGroupsForSource("Analog")).toEqual(["analog"]);
    expect(meterGpioGroupsForSource("Enphase")).toEqual([]);
    expect(meterGpioGroupsForSource("UxIx2")).toEqual([]);
    expect(uartPresenceIdForSource("JsyMk194")).toBe("jsy_mk194");
    expect(uartPresenceIdForSource("UxIx2")).toBeUndefined();
  });

  it("shouldShowMeterPinFilterHint when a single GPIO group is active", () => {
    expect(shouldShowMeterPinFilterHint("JsyMk194")).toBe(true);
    expect(shouldShowMeterPinFilterHint("Analog")).toBe(true);
    expect(shouldShowMeterPinFilterHint("Enphase")).toBe(false);
  });

  it("shows uart pins only for active uart meter", () => {
    expect(isMeterPinActiveForSource("pin_uart_rx", "JsyMk194")).toBe(true);
    expect(isMeterPinActiveForSource("pin_uart_tx", "JsyMk194")).toBe(true);
    expect(isMeterPinActiveForSource("pin_analog0", "JsyMk194")).toBe(false);
    expect(isMeterPinActiveForSource("pin_analog0", "Analog")).toBe(true);
    expect(isMeterPinActiveForSource("pin_uart_rx", "Analog")).toBe(false);
    expect(isMeterPinActiveForSource("pin_uart_rx", "Enphase")).toBe(false);
  });

  it("hides meter pins when source is not compiled in firmware", () => {
    deviceInfo.set({
      router_name: "x",
      firmware_version: "1",
      probe_house_name: "",
      probe_second_name: "",
      temperature_label: "",
      capabilities: {
        firmware_capabilities: { meters: ["JsyMk194"] },
      },
    });
    expect(isMeterCompiledForSource("JsyMk194")).toBe(true);
    expect(isMeterCompiledForSource("Linky")).toBe(false);
    expect(isMeterPinActiveForSource("pin_uart_rx", "JsyMk194")).toBe(true);
    expect(isMeterPinActiveForSource("pin_uart_rx", "Linky")).toBe(false);
    expect(isMeterPinActiveForSource("pin_analog0", "Analog")).toBe(false);
  });

  it("summary rows follow active meter and compile gate", () => {
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
    const analog = HARDWARE_PIN_SUMMARY_ROWS.find((r) => r.configKey === "pin_analog0")!;
    expect(isSummaryRowActiveForSource(uart, "JsyMk194")).toBe(true);
    expect(isSummaryRowActiveForSource(analog, "JsyMk194")).toBe(false);
    expect(isSummaryRowActiveForSource(uart, "Linky")).toBe(false);
  });

  it("treats unknown sources as not compiled", () => {
    deviceInfo.set({
      router_name: "x",
      firmware_version: "1",
      probe_house_name: "",
      probe_second_name: "",
      temperature_label: "",
      capabilities: {
        firmware_capabilities: { meters: ["JsyMk194"] },
      },
    });
    expect(isMeterCompiledForSource("UxIx2")).toBe(false);
    expect(isMeterPinActiveForSource("pin_uart_rx", "UxIx2")).toBe(false);
  });
});
