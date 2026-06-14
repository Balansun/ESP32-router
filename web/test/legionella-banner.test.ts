import { describe, expect, it } from "vitest";
import { resolveLegionellaBannerState } from "../src/utils/legionellaBannerState";

describe("resolveLegionellaBannerState", () => {
  it("hides when triac gating is enabled on at least one period", () => {
    expect(
      resolveLegionellaBannerState({
        gatingDisabled: false,
        temperatureC: -127,
      }),
    ).toBe("hidden");
  });

  it("hides when gating off and probe absent", () => {
    expect(
      resolveLegionellaBannerState({
        gatingDisabled: true,
        temperatureC: -127,
      }),
    ).toBe("hidden");
  });

  it("shows sensor-present warning when probe valid and gating off", () => {
    expect(
      resolveLegionellaBannerState({
        gatingDisabled: true,
        temperatureC: 45,
      }),
    ).toBe("sensor_present");
  });

  it("treats temperature at -100 as absent probe", () => {
    expect(
      resolveLegionellaBannerState({
        gatingDisabled: true,
        temperatureC: -100,
      }),
    ).toBe("hidden");
  });

  it("treats temperature above -100 as present probe", () => {
    expect(
      resolveLegionellaBannerState({
        gatingDisabled: true,
        temperatureC: -99,
      }),
    ).toBe("sensor_present");
  });
});
