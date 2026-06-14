import { describe, expect, it, vi } from "vitest";
import { en } from "../src/i18n/locales/en";
import * as zdt from "../src/utils/zonedDateTime";
import { buildVacationEndField } from "../src/components/VacationEndField";

describe("VacationEndField", () => {
  it("round-trips epoch in Europe/Paris", () => {
    const field = buildVacationEndField({
      T: en,
      initialEpoch: 1_700_000_000,
      getTimeZone: () => "Europe/Paris",
    });
    expect(field.readEpoch()).toBeGreaterThan(0);
    field.writeEpoch(1_800_000_000);
    expect(field.readEpoch()).toBeGreaterThan(0);
    field.refreshTzHint();
    expect(field.el.textContent).toContain("Europe/Paris");
  });

  it("validate accepts empty input", () => {
    const field = buildVacationEndField({
      T: en,
      initialEpoch: 0,
      getTimeZone: () => "Europe/Paris",
    });
    expect(field.validate()).toBe(true);
  });

  it("readEpoch returns zero for empty input", () => {
    const field = buildVacationEndField({
      T: en,
      initialEpoch: 0,
      getTimeZone: () => "Europe/Paris",
    });
    field.input.value = "";
    expect(field.readEpoch()).toBe(0);
  });

  it("validate rejects when epoch conversion returns null", () => {
    vi.spyOn(zdt, "datetimeLocalValueToEpochSec").mockReturnValue(null);
    const field = buildVacationEndField({
      T: en,
      initialEpoch: 0,
      getTimeZone: () => "Europe/Paris",
    });
    field.input.value = "2026-01-01T12:00";
    expect(field.validate()).toBe(false);
    expect(field.readEpoch()).toBe(0);
    vi.restoreAllMocks();
  });
});
