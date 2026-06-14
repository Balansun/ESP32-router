import type { RouteCleanup } from "../../router";

let layoutUpdater: ((path: string) => void) | null = null;
let layoutCleanup: RouteCleanup | null = null;

export function setSettingsLayoutHandlers(
  updater: ((path: string) => void) | null,
  cleanup: RouteCleanup | null,
): void {
  layoutUpdater = updater;
  layoutCleanup = cleanup;
}

export function tryUpdateSettingsLayout(path: string): boolean {
  if (!layoutUpdater) return false;
  layoutUpdater(path);
  return true;
}

export function getSettingsLayoutCleanup(): RouteCleanup | null {
  return layoutCleanup;
}
