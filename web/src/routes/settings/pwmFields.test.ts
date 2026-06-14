import { describe, expect, it } from "vitest";
import { getStrings } from "../../i18n";
import {
  normalizePwmGpio,
  normalizePwmMode,
  pwmGpioOptions,
  pwmModeOptions,
} from "./pwmFields";

describe("pwmFields", () => {
  it("normalizePwmMode falls back to off for unknown values", () => {
    expect(normalizePwmMode("independent")).toBe("independent");
    expect(normalizePwmMode("bogus")).toBe("off");
    expect(normalizePwmMode(undefined)).toBe("off");
  });

  it("normalizePwmGpio falls back to -1 for unknown values", () => {
    expect(normalizePwmGpio(14)).toBe(14);
    expect(normalizePwmGpio(99)).toBe(-1);
    expect(normalizePwmGpio(undefined)).toBe(-1);
  });

  it("pwmModeOptions and pwmGpioOptions use i18n labels", () => {
    const T = getStrings().settings;
    expect(pwmModeOptions(T).map((o) => o.value)).toEqual([
      "off",
      "follow_triac",
      "independent",
    ]);
    expect(pwmGpioOptions(T)[0]).toEqual({ value: "-1", label: T.pwmGpioOff });
    expect(pwmGpioOptions(T).some((o) => o.value === "14" && o.label === "GPIO 14")).toBe(true);
  });
});
