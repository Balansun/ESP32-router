import { scheduleDeviceFetch } from "../api/requestScheduler";
import { isSameReleaseTag } from "../firmware/versionCompare";

const RELOAD_KEY = "balansun_ui_reload";
const LAST_FW_KEY = "balansun_last_fw_version";
const PUBLIC_TIMEOUT_MS = 5000;

export interface PublicDeviceResponse {
  device?: {
    firmware_version?: string;
  };
}

export function shouldSkipUiCacheCheck(embeddedVersion: string): boolean {
  const v = embeddedVersion.trim();
  return !v || v === "dev" || v.includes("%VITE_");
}

export function readEmbeddedUiVersion(doc: Document = document): string {
  const meta = doc.querySelector<HTMLMetaElement>('meta[name="balansun-ui-version"]');
  const fromMeta = meta?.content?.trim();
  if (fromMeta && !fromMeta.includes("%VITE_")) return fromMeta;
  const fromEnv = import.meta.env.VITE_FIRMWARE_VERSION?.trim();
  if (fromEnv && fromEnv !== "dev") return fromEnv;
  return fromMeta || fromEnv || "dev";
}

type DeviceFetchFn = (input: string, init?: RequestInit) => Promise<Response>;

export async function fetchPublicFirmwareVersion(
  fetchFn: DeviceFetchFn = scheduleDeviceFetch,
  timeoutMs = PUBLIC_TIMEOUT_MS,
): Promise<string | null> {
  const ctrl = new AbortController();
  const timer = setTimeout(() => ctrl.abort(), timeoutMs);
  try {
    const res = await fetchFn("/api/v1/public", {
      signal: ctrl.signal,
      cache: "no-store",
    });
    if (!res.ok) return null;
    const data = (await res.json()) as PublicDeviceResponse;
    return data.device?.firmware_version?.trim() || null;
  } catch {
    return null;
  } finally {
    clearTimeout(timer);
  }
}

export interface UiCacheRefreshDecision {
  action: "continue" | "reload";
  reloadUrl?: string;
}

export function decideUiCacheRefresh(
  embeddedVersion: string,
  apiVersion: string | null,
  reloadAttempted: boolean,
): UiCacheRefreshDecision {
  if (shouldSkipUiCacheCheck(embeddedVersion)) {
    return { action: "continue" };
  }
  if (!apiVersion) {
    return { action: "continue" };
  }
  if (isSameReleaseTag(embeddedVersion, apiVersion)) {
    return { action: "continue" };
  }
  if (!reloadAttempted) {
    const path = location.pathname || "/";
    const sep = path.includes("?") ? "&" : "?";
    return {
      action: "reload",
      reloadUrl: `${path}${sep}ui=${encodeURIComponent(apiVersion)}`,
    };
  }
  return { action: "continue" };
}

function readReloadAttempted(storage: Storage): boolean {
  try {
    return storage.getItem(RELOAD_KEY) === "1";
  } catch {
    return false;
  }
}

function markReloadAttempted(storage: Storage): void {
  try {
    storage.setItem(RELOAD_KEY, "1");
  } catch {
    /* ignore */
  }
}

function clearReloadAttempted(storage: Storage): void {
  try {
    storage.removeItem(RELOAD_KEY);
  } catch {
    /* ignore */
  }
}

function persistLastFirmwareVersion(storage: Storage, version: string): void {
  try {
    storage.setItem(LAST_FW_KEY, version);
  } catch {
    /* ignore */
  }
}

/** Reload when cached UI bundle version disagrees with live firmware (installed PWA). */
export async function ensureUiFresh(): Promise<void> {
  const embedded = readEmbeddedUiVersion();
  if (shouldSkipUiCacheCheck(embedded)) return;

  const apiVersion = await fetchPublicFirmwareVersion();
  const reloadAttempted = readReloadAttempted(sessionStorage);
  const decision = decideUiCacheRefresh(embedded, apiVersion, reloadAttempted);

  if (decision.action === "reload" && decision.reloadUrl) {
    markReloadAttempted(sessionStorage);
    location.replace(decision.reloadUrl);
    await new Promise<void>(() => {
      /* block until navigation */
    });
    return;
  }

  clearReloadAttempted(sessionStorage);
  if (apiVersion && isSameReleaseTag(embedded, apiVersion)) {
    persistLastFirmwareVersion(localStorage, apiVersion);
  }
}
