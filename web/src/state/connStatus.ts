import { api } from "../api/client";
import { normalizeLogicalPath } from "../paths";
import { WIFI_AP_PATH } from "../wifi/wifiPaths";
import type { ConnState } from "./store";
import { conn } from "./store";

const FAIL_THRESHOLD = 3;
const HEALTH_INTERVAL_MS = 8000;

let failStreak = 0;
let healthTimer: number | undefined;
let healthAbort: AbortController | null = null;

/** App-bar connection LED: lightweight /health only (not heavy poll endpoints). */
export function startConnHealthMonitor(): () => void {
  stopConnHealthMonitor();
  if (
    typeof location !== "undefined" &&
    normalizeLogicalPath(location.pathname || "/") === WIFI_AP_PATH
  ) {
    return () => {};
  }

  const ctrl = new AbortController();
  healthAbort = ctrl;

  const run = async () => {
    if (ctrl.signal.aborted) return;
    try {
      await api.getHealth({ signal: ctrl.signal, timeoutMs: 4000 });
      failStreak = 0;
      conn.set("ok");
    } catch {
      if (ctrl.signal.aborted) return;
      failStreak++;
      if (failStreak >= FAIL_THRESHOLD) {
        conn.set("err");
      }
    }
    if (!ctrl.signal.aborted) {
      healthTimer = window.setTimeout(() => void run(), HEALTH_INTERVAL_MS);
    }
  };

  queueMicrotask(run);

  return stopConnHealthMonitor;
}

export function stopConnHealthMonitor(): void {
  healthAbort?.abort();
  healthAbort = null;
  if (healthTimer !== undefined) {
    clearTimeout(healthTimer);
    healthTimer = undefined;
  }
}

/** Poll outcomes do not drive the connection LED (see health monitor). */
export function reportPollSuccess(): void {}

export function reportPollFailure(): void {}

export function resetConnStatus(state: ConnState = "loading"): void {
  failStreak = 0;
  conn.set(state);
}
