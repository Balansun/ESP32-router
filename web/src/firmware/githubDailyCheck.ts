import { api } from "../api/client";
import { isAuthExemptPath } from "../auth/httpAuthGate";
import { formatFirmwareVersionFull, isNewerReleaseAvailable } from "./versionCompare";
import { deviceInfo } from "../state/store";
import { firmwareUpdate } from "../state/firmwareUpdate";
import {
  checkGithubRelease,
  GithubReleaseError,
  GITHUB_RELEASES_PAGE,
  loadStoredChannel,
  loadStoredReleaseCheck,
  markDailyAutoCheck,
  shouldRunDailyAutoCheck,
  storeReleaseCheck,
  storedToReleaseCheck,
  type ReleaseCheckResult,
} from "./githubRelease";

export function formatAvailableReleaseLabel(releaseTag: string): string {
  const full = formatFirmwareVersionFull(releaseTag);
  if (full === "—") return "";
  return full;
}

function applyCheckToUi(result: ReleaseCheckResult): void {
  const available = isNewerReleaseAvailable(
    deviceInfo.get()?.firmware_version ?? "0",
    result.release.tag_name,
  );
  firmwareUpdate.set({
    available,
    releaseTag: result.release.tag_name,
    checking: false,
    compare: result.compare,
    lastCheckedAt: new Date().toISOString(),
    error: "",
  });
}

export function hydrateFirmwareUpdateFromStorage(): void {
  const channel = loadStoredChannel();
  let stored = loadStoredReleaseCheck(channel);
  if (!stored && channel === "prerelease") {
    stored = loadStoredReleaseCheck("stable");
  }
  const version = deviceInfo.get()?.firmware_version ?? "0";
  if (!stored) {
    firmwareUpdate.set({
      available: false,
      releaseTag: "",
      checking: false,
      compare: null,
      error: "",
    });
    return;
  }
  const hydrated = storedToReleaseCheck(stored, channel, version, undefined);
  if (!hydrated) {
    firmwareUpdate.set({
      available: false,
      releaseTag: "",
      checking: false,
      compare: null,
      error: "",
    });
    return;
  }
  applyCheckToUi(hydrated);
}

async function resolveChipModel(): Promise<string | undefined> {
  try {
    const d = await api.getDevice({ timeoutMs: 8000, retry: 1 });
    return d.chip?.model;
  } catch {
    return undefined;
  }
}

export type FirmwareUpdateCheckOutcome =
  | { ok: true; result: ReleaseCheckResult }
  | { ok: false; error: unknown };

/** Query GitHub for the selected channel (ignores the 24 h throttle when `force` is true). */
export async function runFirmwareUpdateCheck(opts?: {
  force?: boolean;
}): Promise<FirmwareUpdateCheckOutcome> {
  if (typeof location !== "undefined" && isAuthExemptPath(location.pathname || "/")) {
    return { ok: false, error: new Error("auth_exempt") };
  }

  const channel = loadStoredChannel();
  if (!opts?.force && !shouldRunDailyAutoCheck(channel)) {
    hydrateFirmwareUpdateFromStorage();
    return { ok: false, error: new Error("skipped_daily") };
  }

  firmwareUpdate.set({ checking: true, error: "" });

  const version = deviceInfo.get()?.firmware_version ?? "0";
  const chipModel = await resolveChipModel();
  try {
    const result = await checkGithubRelease(channel, version, chipModel);
    storeReleaseCheck(result);
    markDailyAutoCheck(channel);
    applyCheckToUi(result);
    return { ok: true, result };
  } catch (e) {
    firmwareUpdate.set({ checking: false, error: "check_failed" });
    return { ok: false, error: e };
  }
}

/** Once per 24 h per channel: check GitHub releases and refresh the app-bar badge. */
export async function runFirmwareUpdateDailyCheck(): Promise<void> {
  await runFirmwareUpdateCheck();
}

export function firmwareUpdateStatusMessage(
  state: ReturnType<typeof firmwareUpdate.get>,
  strings: {
    checking: string;
    available: string;
    upToDate: string;
    newerThanRelease: string;
    checkError: string;
    notChecked: string;
  },
  currentVersion: string,
): string {
  if (state.checking) return strings.checking;
  if (state.error) return strings.checkError;
  if (state.compare === null) return strings.notChecked;

  const currentFull = formatFirmwareVersionFull(currentVersion);
  const releaseFull = formatFirmwareVersionFull(state.releaseTag);

  if (state.compare < 0 && state.releaseTag) {
    return strings.available
      .replaceAll("{current}", currentFull)
      .replaceAll("{release}", releaseFull);
  }
  if (state.compare === 0) {
    return strings.upToDate
      .replaceAll("{current}", currentFull)
      .replaceAll("{release}", releaseFull || "—");
  }
  if (state.compare > 0) {
    return strings.newerThanRelease
      .replaceAll("{current}", currentFull)
      .replaceAll("{release}", releaseFull || "—");
  }
  return strings.notChecked;
}

export function githubUpdateCheckErrorMessage(e: unknown, strings: {
  noPrerelease: string;
  rateLimit: string;
  repoNotFound: string;
  network: string;
  generic: string;
}): string {
  if (e instanceof GithubReleaseError) {
    switch (e.code) {
      case "no_prerelease":
        return strings.noPrerelease;
      case "rate_limit":
        return strings.rateLimit;
      case "repo_not_found":
        return strings.repoNotFound;
      case "github_fetch_failed":
        return strings.network;
      default:
        return strings.generic;
    }
  }
  return strings.generic;
}

export { GITHUB_RELEASES_PAGE };
