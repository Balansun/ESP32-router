import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { deviceInfo } from "../src/state/store";
import { firmwareUpdate } from "../src/state/firmwareUpdate";
import {
  GithubReleaseError,
  FW_AUTO_CHECK_STORAGE_KEY,
  FW_CHECK_STORAGE_KEY,
  FW_CHANNEL_STORAGE_KEY,
  releaseCheckToStored,
  type ReleaseCheckResult,
} from "../src/firmware/githubRelease";

const checkGithubReleaseMock = vi.hoisted(() => vi.fn());
const shouldRunDailyAutoCheckMock = vi.hoisted(() => vi.fn(() => true));
const apiMock = vi.hoisted(() => ({
  getDevice: vi.fn(),
}));

vi.mock("../src/api/client", () => ({ api: apiMock }));
vi.mock("../src/firmware/githubRelease", async (importOriginal) => {
  const actual = await importOriginal<typeof import("../src/firmware/githubRelease")>();
  return {
    ...actual,
    checkGithubRelease: checkGithubReleaseMock,
    shouldRunDailyAutoCheck: shouldRunDailyAutoCheckMock,
  };
});

import {
  formatAvailableReleaseLabel,
  firmwareUpdateStatusMessage,
  githubUpdateCheckErrorMessage,
  hydrateFirmwareUpdateFromStorage,
  runFirmwareUpdateCheck,
} from "../src/firmware/githubDailyCheck";

const STATUS_STRINGS = {
  checking: "Checking…",
  available: "Update {release} ({current})",
  upToDate: "Up to date {current} / {release}",
  newerThanRelease: "Ahead {current} / {release}",
  checkError: "Check failed",
  notChecked: "Not checked",
};

const ERROR_STRINGS = {
  noPrerelease: "No prerelease",
  rateLimit: "Rate limit",
  repoNotFound: "Not found",
  network: "Network",
  generic: "Generic",
};

const CHECK_RESULT: ReleaseCheckResult = {
  channel: "prerelease",
  boardEnv: "wroom32",
  compare: -1,
  release: {
    tag_name: "v0.2.0",
    name: "0.2.0",
    prerelease: false,
    draft: false,
    published_at: "2026-04-01T00:00:00Z",
    assets: [],
  },
  firmwareAsset: {
    name: "balansun-0.2.0-wroom32-firmware.bin",
    size: 1,
    url: "https://api.github.com/assets/1",
    browser_download_url: "https://example.com/fw.bin",
  },
  checksumsAsset: {
    name: "SHA256SUMS.txt",
    size: 1,
    url: "https://api.github.com/assets/2",
    browser_download_url: "https://example.com/sums.txt",
  },
};

describe("githubDailyCheck", () => {
  const lsBacking = new Map<string, string>();

  beforeEach(() => {
    lsBacking.clear();
    vi.stubGlobal("localStorage", {
      getItem: (k: string) => lsBacking.get(k) ?? null,
      setItem: (k: string, v: string) => {
        lsBacking.set(k, v);
      },
      removeItem: (k: string) => {
        lsBacking.delete(k);
      },
    });
    firmwareUpdate.reset();
    deviceInfo.set({ firmware_version: "0.1.0" } as never);
    checkGithubReleaseMock.mockReset();
    shouldRunDailyAutoCheckMock.mockReturnValue(true);
    apiMock.getDevice.mockResolvedValue({ chip: { model: "ESP32-D0WD-V3" } });
    vi.stubGlobal("location", { pathname: "/settings" });
  });

  afterEach(() => {
    vi.unstubAllGlobals();
  });

  it("formatAvailableReleaseLabel formats release tag for display", () => {
    expect(formatAvailableReleaseLabel("v0.2.0")).toBe("v0.2.0");
    expect(formatAvailableReleaseLabel("")).toBe("");
  });

  it("firmwareUpdateStatusMessage covers all states", () => {
    expect(
      firmwareUpdateStatusMessage(
        { checking: true, error: "", compare: null, releaseTag: "" } as never,
        STATUS_STRINGS,
        "0.1.0",
      ),
    ).toBe(STATUS_STRINGS.checking);
    expect(
      firmwareUpdateStatusMessage(
        { checking: false, error: "x", compare: 0, releaseTag: "v0.2.0" } as never,
        STATUS_STRINGS,
        "0.1.0",
      ),
    ).toBe(STATUS_STRINGS.checkError);
    expect(
      firmwareUpdateStatusMessage(
        { checking: false, error: "", compare: null, releaseTag: "" } as never,
        STATUS_STRINGS,
        "0.1.0",
      ),
    ).toBe(STATUS_STRINGS.notChecked);
    expect(
      firmwareUpdateStatusMessage(
        { checking: false, error: "", compare: -1, releaseTag: "v0.2.0" } as never,
        STATUS_STRINGS,
        "0.1.0",
      ),
    ).toContain("0.2.0");
    expect(
      firmwareUpdateStatusMessage(
        { checking: false, error: "", compare: 0, releaseTag: "v0.2.0" } as never,
        STATUS_STRINGS,
        "0.1.0",
      ),
    ).toContain("Up to date");
    expect(
      firmwareUpdateStatusMessage(
        { checking: false, error: "", compare: 1, releaseTag: "v0.1.0" } as never,
        STATUS_STRINGS,
        "0.2.0",
      ),
    ).toContain("Ahead");
  });

  it("githubUpdateCheckErrorMessage maps GithubReleaseError codes", () => {
    expect(
      githubUpdateCheckErrorMessage(
        new GithubReleaseError("no_prerelease"),
        ERROR_STRINGS,
      ),
    ).toBe(ERROR_STRINGS.noPrerelease);
    expect(
      githubUpdateCheckErrorMessage(new GithubReleaseError("rate_limit"), ERROR_STRINGS),
    ).toBe(ERROR_STRINGS.rateLimit);
    expect(
      githubUpdateCheckErrorMessage(
        new GithubReleaseError("repo_not_found"),
        ERROR_STRINGS,
      ),
    ).toBe(ERROR_STRINGS.repoNotFound);
    expect(
      githubUpdateCheckErrorMessage(
        new GithubReleaseError("github_fetch_failed"),
        ERROR_STRINGS,
      ),
    ).toBe(ERROR_STRINGS.network);
    expect(
      githubUpdateCheckErrorMessage(new GithubReleaseError("asset_not_found"), ERROR_STRINGS),
    ).toBe(ERROR_STRINGS.generic);
    expect(githubUpdateCheckErrorMessage(new Error("x"), ERROR_STRINGS)).toBe(
      ERROR_STRINGS.generic,
    );
  });

  it("hydrateFirmwareUpdateFromStorage clears state when nothing stored", () => {
    localStorage.setItem(FW_CHANNEL_STORAGE_KEY, "stable");
    hydrateFirmwareUpdateFromStorage();
    expect(firmwareUpdate.get().compare).toBeNull();
    expect(firmwareUpdate.get().available).toBe(false);
  });

  it("hydrateFirmwareUpdateFromStorage applies stored check", () => {
    localStorage.setItem(FW_CHANNEL_STORAGE_KEY, "stable");
    localStorage.setItem(
      `${FW_CHECK_STORAGE_KEY}:stable`,
      JSON.stringify(releaseCheckToStored(CHECK_RESULT)),
    );
    hydrateFirmwareUpdateFromStorage();
    expect(firmwareUpdate.get().compare).toBe(-1);
    expect(firmwareUpdate.get().available).toBe(true);
    expect(firmwareUpdate.get().releaseTag).toBe("v0.2.0");
  });

  it("runFirmwareUpdateCheck skips on auth-exempt paths", async () => {
    vi.stubGlobal("location", { pathname: "/wifi" });
    const out = await runFirmwareUpdateCheck({ force: true });
    expect(out.ok).toBe(false);
    expect((out as { error: Error }).error.message).toBe("auth_exempt");
  });

  it("runFirmwareUpdateCheck returns skipped_daily when throttled", async () => {
    shouldRunDailyAutoCheckMock.mockReturnValue(false);
    const out = await runFirmwareUpdateCheck();
    expect(out.ok).toBe(false);
    expect((out as { error: Error }).error.message).toBe("skipped_daily");
  });

  it("runFirmwareUpdateCheck stores result and marks available when newer", async () => {
    checkGithubReleaseMock.mockResolvedValue(CHECK_RESULT);
    const out = await runFirmwareUpdateCheck({ force: true });
    expect(out.ok).toBe(true);
    expect(firmwareUpdate.get().available).toBe(true);
    expect(firmwareUpdate.get().checking).toBe(false);
    expect(lsBacking.has(`${FW_CHECK_STORAGE_KEY}:prerelease`)).toBe(true);
    expect(lsBacking.get(FW_AUTO_CHECK_STORAGE_KEY)).toBeTruthy();
  });

  it("runFirmwareUpdateCheck handles API failures", async () => {
    checkGithubReleaseMock.mockRejectedValue(new GithubReleaseError("rate_limit"));
    const out = await runFirmwareUpdateCheck({ force: true });
    expect(out.ok).toBe(false);
    expect(firmwareUpdate.get().checking).toBe(false);
    expect(firmwareUpdate.get().error).toBe("check_failed");
  });

  it("runFirmwareUpdateCheck tolerates getDevice failure", async () => {
    apiMock.getDevice.mockRejectedValue(new Error("offline"));
    checkGithubReleaseMock.mockResolvedValue({ ...CHECK_RESULT, compare: 0 });
    deviceInfo.set({ firmware_version: "0.2.0" } as never);
    const out = await runFirmwareUpdateCheck({ force: true });
    expect(out.ok).toBe(true);
    expect(firmwareUpdate.get().available).toBe(false);
  });
});
