import { describe, expect, it } from "vitest";
import { isSafetyLockoutActive } from "./safetyLockout";
import type { HealthInfo, StateOperationalStatus } from "../api/types";

describe("isSafetyLockoutActive", () => {
  it("returns true when health.safety_lockout.active", () => {
    const health = { safety_lockout: { active: true, reasons: ["zc_failed"] } } as HealthInfo;
    expect(isSafetyLockoutActive(health)).toBe(true);
  });

  it("returns true when health.self_test.safety_lockout_active", () => {
    const health = { self_test: { safety_lockout_active: true } } as HealthInfo;
    expect(isSafetyLockoutActive(health)).toBe(true);
  });

  it("returns true when state output_suspend reason is safety_lockout", () => {
    const status = {
      output_suspend: { active: true, reason: "safety_lockout" },
    } as StateOperationalStatus;
    expect(isSafetyLockoutActive(null, status)).toBe(true);
  });

  it("returns false when no lockout signals", () => {
    expect(isSafetyLockoutActive({}, {})).toBe(false);
  });

  it("returns false when suspend is active but reason is not safety_lockout", () => {
    const status = {
      output_suspend: { active: true, reason: "vacation" },
    } as StateOperationalStatus;
    expect(isSafetyLockoutActive(null, status)).toBe(false);
  });

  it("returns false when safety_lockout object is present but inactive", () => {
    const health = { safety_lockout: { active: false, reasons: [] } } as HealthInfo;
    expect(isSafetyLockoutActive(health)).toBe(false);
  });
});
