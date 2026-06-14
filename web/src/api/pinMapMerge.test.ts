import { describe, expect, it } from "vitest";
import { mergePinFieldsIntoConfig, overlayPinMapOnConfig } from "./pinMapMerge";
import type { HardwarePinMap, RouterConfig } from "./types";

describe("mergePinFieldsIntoConfig", () => {
  it("overlays pin_* from system/pins when config export is sparse", () => {
    const sparse: RouterConfig = { source: "JsyMk194" };
    const pins: HardwarePinMap = {
      pin_triac_dim: 18,
      pin_zero_cross: 19,
      pin_uart_rx: 26,
      pin_uart_tx: 27,
      pin_temp: 13,
      pin_analog0: 35,
      pin_analog1: 32,
      pin_analog2: 33,
      board_pin_profile: "wroom32",
      pin_default_uart_rx: 26,
    };
    const merged = mergePinFieldsIntoConfig(sparse, pins);
    expect(merged.pin_triac_dim).toBe(18);
    expect(merged.pin_zero_cross).toBe(19);
    expect(merged.pin_uart_rx).toBe(26);
    expect(merged.board_pin_profile).toBe("wroom32");
    expect(merged.pin_default_uart_rx).toBe(26);
    expect(merged.source).toBe("JsyMk194");
  });

  it("keeps config values when pins payload omits optional fields", () => {
    const cfg: RouterConfig = {
      pin_triac_dim: 22,
      pin_map_fallback_active: true,
    };
    const pins: HardwarePinMap = {
      pin_triac_dim: 99,
      pin_zero_cross: 23,
      pin_uart_rx: 4,
      pin_uart_tx: 5,
      pin_temp: 21,
      pin_analog0: 1,
      pin_analog1: 2,
      pin_analog2: 3,
    };
    const merged = mergePinFieldsIntoConfig(cfg, pins);
    expect(merged.pin_triac_dim).toBe(99);
    expect(merged.pin_map_fallback_active).toBe(true);
  });

  it("overlayPinMapOnConfig copies only defined pin keys", () => {
    const cfg: RouterConfig = { source: "JsyMk194", pin_triac_dim: 22 };
    const merged = overlayPinMapOnConfig(cfg, { pin_triac_dim: 18, pin_uart_rx: 26 });
    expect(merged.pin_triac_dim).toBe(18);
    expect(merged.pin_uart_rx).toBe(26);
    expect(merged.source).toBe("JsyMk194");
  });
});
