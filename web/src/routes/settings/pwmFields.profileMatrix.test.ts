import { describe, expect, it } from "vitest";
import { getStrings } from "../../i18n";
import {
  PWM_GPIO_CHOICES,
  PWM_MODES,
  normalizePwmGpio,
  normalizePwmMode,
  pwmGpioOptions,
  pwmModeOptions,
} from "./pwmFields";

describe("pwmFields profile matrix invariants", () => {
  it("PWM_MODES match firmware contract", () => {
    expect([...PWM_MODES]).toEqual(["off", "follow_triac", "independent"]);
  });

  it("PWM_GPIO_CHOICES match firmware allow-list", () => {
    expect([...PWM_GPIO_CHOICES]).toEqual([-1, 4, 5, 14, 16, 17, 21, 25]);
  });

  it("option builders cover every mode and gpio", () => {
    const T = getStrings().settings;
    expect(pwmModeOptions(T).map((o) => o.value)).toEqual([...PWM_MODES]);
    expect(pwmGpioOptions(T).map((o) => o.value)).toEqual(
      PWM_GPIO_CHOICES.map(String),
    );
  });

  it("normalizers reject out-of-range values", () => {
    expect(normalizePwmMode("invalid")).toBe("off");
    expect(normalizePwmGpio(99)).toBe(-1);
  });
});
