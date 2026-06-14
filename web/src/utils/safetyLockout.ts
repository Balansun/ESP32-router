import type { HealthInfo, StateOperationalStatus } from "../api/types";

export function isSafetyLockoutActive(
  health?: HealthInfo | null,
  status?: StateOperationalStatus | null,
): boolean {
  if (health?.safety_lockout?.active === true) return true;
  if (health?.self_test?.safety_lockout_active === true) return true;
  if (status?.output_suspend?.active && status.output_suspend.reason === "safety_lockout") {
    return true;
  }
  return false;
}
