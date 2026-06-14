import { describe, expect, it } from "vitest";
import {
  hardwarePinDefaultsForBoardEnv,
  resolveBoardPinProfile,
  resolveHardwarePinDefaults,
} from "./hardwarePinDefaults";

describe("resolveHardwarePinDefaults", () => {
  it("uses firmware-exported defaults when present", () => {
    const cfg = {
      board_pin_profile: "esp32s3" as const,
      pin_default_uart_rx: 4,
      pin_default_uart_tx: 5,
      pin_default_temp: 21,
      pin_default_analog0: 1,
      pin_default_analog1: 2,
      pin_default_analog2: 3,
      pin_default_triac_dim: 22,
      pin_default_zero_cross: 23,
    };
    expect(resolveHardwarePinDefaults(cfg, "wroom32")).toEqual(
      hardwarePinDefaultsForBoardEnv("esp32s3"),
    );
  });

  it("falls back to wroom32 table for classic ESP32", () => {
    expect(resolveHardwarePinDefaults({}, "wroom32")).toEqual(
      hardwarePinDefaultsForBoardEnv("wroom32"),
    );
  });

  it("falls back to esp32s3 table for S3", () => {
    expect(resolveHardwarePinDefaults({}, "esp32s3")).toEqual(
      hardwarePinDefaultsForBoardEnv("esp32s3"),
    );
  });
});

describe("resolveBoardPinProfile", () => {
  it("prefers config board_pin_profile", () => {
    expect(resolveBoardPinProfile({ board_pin_profile: "esp32s3" }, "ESP32")).toBe("esp32s3");
    expect(resolveBoardPinProfile({ board_pin_profile: "wroom32" }, "ESP32-S3")).toBe("wroom32");
  });

  it("detects S3 from chip model when profile missing", () => {
    expect(resolveBoardPinProfile({}, "ESP32-S3")).toBe("esp32s3");
    expect(resolveBoardPinProfile({}, "ESP32-D0WDQ6")).toBe("wroom32");
  });
});
