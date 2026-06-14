import { describe, expect, it } from "vitest";
import {
  isEsp32s3Chip,
  isLegacyWroomDualGpioPreset,
  resolveStatusLedModeForEsp32s3,
} from "../src/routes/settings/statusLedEsp32s3";

describe("statusLedEsp32s3", () => {
  it("detects S3 chip model", () => {
    expect(isEsp32s3Chip("ESP32-S3")).toBe(true);
    expect(isEsp32s3Chip("ESP32")).toBe(false);
    expect(isEsp32s3Chip(undefined)).toBe(false);
  });

  it("recognizes legacy WROOM dual-GPIO preset", () => {
    expect(isLegacyWroomDualGpioPreset("dual_gpio", 18, 19)).toBe(true);
    expect(isLegacyWroomDualGpioPreset("rgb_ws2812", 18, 19)).toBe(false);
    expect(isLegacyWroomDualGpioPreset("dual_gpio", 5, 19)).toBe(false);
  });

  it("maps legacy preset to rgb_ws2812 on S3", () => {
    expect(resolveStatusLedModeForEsp32s3("dual_gpio", 18, 19, true)).toBe("rgb_ws2812");
    expect(resolveStatusLedModeForEsp32s3("dual_gpio", 18, 19, false)).toBe("dual_gpio");
    expect(resolveStatusLedModeForEsp32s3("off", 18, 19, true)).toBe("off");
  });
});
