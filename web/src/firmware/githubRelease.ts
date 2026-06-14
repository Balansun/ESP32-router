import { ApiNetworkError, isBrowserNetworkFailure } from "../api/networkFailure";
import { pickBoardEnv, type BoardEnv } from "./boardEnv";
import { compareFirmwareVersions, parseVersionParts } from "./versionCompare";

/** GitHub releases with this major are offered for in-app OTA (Balansun 0.x line). */
export const PRODUCT_LINE_MAJOR = 0;

/** Override at build time (see web/.env.example) if the repo is renamed or forked. */
export const GITHUB_OWNER = (
  import.meta.env.VITE_GITHUB_OWNER as string | undefined
)?.trim() || "Balansun";
export const GITHUB_REPO = (
  import.meta.env.VITE_GITHUB_REPO as string | undefined
)?.trim() || "ESP32-router";
export const GITHUB_RELEASES_PAGE = `https://github.com/${GITHUB_OWNER}/${GITHUB_REPO}/releases`;
export const FW_CHANNEL_STORAGE_KEY = "balansun_fw_channel";
export const FW_CHECK_STORAGE_KEY = "balansun_fw_github_check";
export const FW_AUTO_CHECK_STORAGE_KEY = "balansun_fw_github_auto_check";

const MS_PER_DAY = 24 * 60 * 60 * 1000;

function checkStorageKey(channel: ReleaseChannel): string {
  return `${FW_CHECK_STORAGE_KEY}:${channel}`;
}

const API_BASE = `https://api.github.com/repos/${GITHUB_OWNER}/${GITHUB_REPO}`;

/** Build GitHub REST headers (anonymous api.github.com). */
export function githubApiHeaders(extra?: Record<string, string>): Record<string, string> {
  return {
    Accept: "application/vnd.github+json",
    "X-GitHub-Api-Version": "2022-11-28",
    ...extra,
  };
}

export type ReleaseChannel = "stable" | "prerelease";

export type GithubReleaseErrorCode =
  | "no_stable_release"
  | "no_prerelease"
  | "asset_not_found"
  | "checksums_missing"
  | "checksum_line_missing"
  | "checksum_mismatch"
  | "rate_limit"
  | "repo_not_found"
  | "github_fetch_failed"
  | "download_failed";

export class GithubReleaseError extends Error {
  readonly code: GithubReleaseErrorCode;

  constructor(code: GithubReleaseErrorCode, message?: string) {
    super(message ?? code);
    this.name = "GithubReleaseError";
    this.code = code;
  }
}

export interface GhAsset {
  name: string;
  size: number;
  browser_download_url: string;
  url: string;
}

/** Primary URL for device OTA (API asset); firmware also receives `firmware_browser_url` as fallback. */
export function assetDownloadUrlForDevice(asset: GhAsset): string {
  return asset.url?.trim() || asset.browser_download_url?.trim() || "";
}

export interface GhRelease {
  tag_name: string;
  name: string;
  prerelease: boolean;
  draft: boolean;
  published_at: string;
  assets: GhAsset[];
}

export function loadStoredChannel(): ReleaseChannel {
  try {
    const v =
      localStorage.getItem(FW_CHANNEL_STORAGE_KEY) ??
      sessionStorage.getItem(FW_CHANNEL_STORAGE_KEY);
    if (v === "stable") return "prerelease";
    if (v === "prerelease" || v === "nightly") return "prerelease";
  } catch {
    /* ignore */
  }
  return "prerelease";
}

export function storeChannel(channel: ReleaseChannel): void {
  try {
    localStorage.setItem(FW_CHANNEL_STORAGE_KEY, channel);
    sessionStorage.setItem(FW_CHANNEL_STORAGE_KEY, channel);
  } catch {
    /* ignore */
  }
}

/** True when tag is part of the current Balansun semver line (e.g. v0.1.0). */
export function isProductLineReleaseTag(tag: string): boolean {
  const parts = parseVersionParts(tag);
  return parts.length > 0 && parts[0] === PRODUCT_LINE_MAJOR;
}

export function resolveRelease(
  channel: ReleaseChannel,
  releases: GhRelease[],
): GhRelease {
  if (channel === "stable") {
    const r = releases.find((x) => !x.draft && !x.prerelease);
    if (!r) throw new GithubReleaseError("no_stable_release");
    return r;
  }
  const r = releases.find((x) => !x.draft && x.prerelease);
  if (!r) throw new GithubReleaseError("no_prerelease");
  return r;
}

export function versionFromTag(tag: string): string {
  return tag.replace(/^v/i, "");
}

export function firmwareAssetName(version: string, env: BoardEnv): string {
  return `balansun-${version}-${env}-firmware.bin`;
}

export function findFirmwareAsset(release: GhRelease, env: BoardEnv): GhAsset {
  const name = firmwareAssetName(versionFromTag(release.tag_name), env);
  const asset = release.assets.find((a) => a.name === name);
  if (!asset) throw new GithubReleaseError("asset_not_found");
  return asset;
}

export function findChecksumsAsset(release: GhRelease): GhAsset | undefined {
  return release.assets.find((a) => a.name === "SHA256SUMS.txt");
}

/** Parse `sha256sum` line for a given firmware filename. */
export function parseSha256ForFile(
  sumsText: string,
  filename: string,
): string | null {
  for (const line of sumsText.split("\n")) {
    const trimmed = line.trim();
    if (!trimmed) continue;
    const m = trimmed.match(/^([0-9a-fA-F]{64})\s+(.+)$/);
    if (!m) continue;
    const hash = m[1].toLowerCase();
    const path = m[2].trim();
    if (path === filename || path.endsWith(`/${filename}`)) return hash;
  }
  return null;
}

export async function sha256Hex(buffer: ArrayBuffer): Promise<string> {
  const hash = await crypto.subtle.digest("SHA-256", buffer);
  return [...new Uint8Array(hash)]
    .map((b) => b.toString(16).padStart(2, "0"))
    .join("");
}

export async function verifyFirmwareSha256(
  firmware: ArrayBuffer,
  sumsText: string,
  filename: string,
): Promise<void> {
  const expected = parseSha256ForFile(sumsText, filename);
  if (!expected) throw new GithubReleaseError("checksum_line_missing");
  const actual = await sha256Hex(firmware);
  if (actual !== expected) throw new GithubReleaseError("checksum_mismatch");
}

function mapGhApiFailure(res: Response): GithubReleaseError {
  if (res.status === 403) return new GithubReleaseError("rate_limit");
  if (res.status === 404) return new GithubReleaseError("repo_not_found");
  return new GithubReleaseError("github_fetch_failed");
}

function isRedirectStatus(status: number): boolean {
  return status === 301 || status === 302 || status === 303 || status === 307 || status === 308;
}

/**
 * GitHub release assets redirect to blob storage. Browsers fail CORS when
 * `Authorization` is forwarded on that redirect — resolve the blob URL first,
 * then download without auth (public release assets are world-readable).
 */
/**
 * Resolve the blob/CDN URL GitHub returns after redirects (browser can do this; ESP32
 * should download this URL directly to avoid multi-hop redirect failures).
 */
export async function resolveGitHubAssetCdnUrl(apiUrl: string): Promise<string | null> {
  const trimmed = apiUrl.trim();
  if (!trimmed) return null;
  let current = trimmed;
  for (let hop = 0; hop < 10; hop++) {
    const res = await fetch(current, {
      method: "HEAD",
      redirect: "manual",
      headers: githubApiHeaders({ Accept: "application/octet-stream" }),
    });
    if (res.status === 403) throw new GithubReleaseError("rate_limit");
    if (isRedirectStatus(res.status)) {
      const loc = res.headers.get("Location")?.trim();
      if (!loc) return null;
      try {
        current = new URL(loc, current).href;
      } catch {
        return null;
      }
      continue;
    }
    if (res.ok || res.status === 405) return current;
    return null;
  }
  return null;
}

async function fetchReleaseAssetBytes(apiUrl: string): Promise<ArrayBuffer> {
  const probe = await fetch(apiUrl, {
    headers: githubApiHeaders({ Accept: "application/octet-stream" }),
    redirect: "manual",
  });
  if (probe.status === 403) throw new GithubReleaseError("rate_limit");

  if (isRedirectStatus(probe.status)) {
    const blobUrl = probe.headers.get("Location")?.trim();
    if (!blobUrl) throw new GithubReleaseError("download_failed");
    const res = await fetch(blobUrl, { redirect: "follow" });
    if (!res.ok) throw new GithubReleaseError("download_failed");
    return res.arrayBuffer();
  }

  if (probe.ok) return probe.arrayBuffer();
  throw new GithubReleaseError("download_failed");
}

async function ghFetch(path: string): Promise<Response> {
  try {
    return await fetch(`${API_BASE}${path}`, {
      headers: githubApiHeaders(),
    });
  } catch (e) {
    if (isBrowserNetworkFailure(e)) {
      throw new ApiNetworkError("github_check");
    }
    throw e;
  }
}

async function fetchReleasesList(): Promise<GhRelease[]> {
  const res = await ghFetch("/releases?per_page=30");
  if (!res.ok) throw mapGhApiFailure(res);
  return res.json() as Promise<GhRelease[]>;
}

export async function fetchReleaseForChannel(
  channel: ReleaseChannel,
): Promise<GhRelease> {
  const list = await fetchReleasesList();
  return resolveRelease(channel, list);
}

/** Download a release asset via the GitHub REST API (unit tests). */
export async function downloadReleaseAsset(asset: GhAsset): Promise<ArrayBuffer> {
  const apiUrl = asset.url?.trim();
  if (!apiUrl) {
    throw new GithubReleaseError("download_failed");
  }
  return fetchReleaseAssetBytes(apiUrl);
}

export interface ReleaseCheckResult {
  release: GhRelease;
  channel: ReleaseChannel;
  boardEnv: BoardEnv;
  firmwareAsset: GhAsset;
  checksumsAsset: GhAsset;
  /** Current device version vs release tag. */
  compare: -1 | 0 | 1;
}

interface StoredReleaseCheckV1 {
  v: 1;
  channel: ReleaseChannel;
  checkedAt: string;
  boardEnv: BoardEnv;
  compare: -1 | 0 | 1;
  release: Pick<GhRelease, "tag_name" | "name" | "prerelease" | "published_at" | "draft">;
  firmwareAsset: GhAsset;
  checksumsAsset: GhAsset;
}

function releaseFromStored(stored: StoredReleaseCheckV1): GhRelease {
  return {
    ...stored.release,
    assets: [stored.firmwareAsset, stored.checksumsAsset],
  };
}

export function releaseCheckToStored(result: ReleaseCheckResult): StoredReleaseCheckV1 {
  return {
    v: 1,
    channel: result.channel,
    checkedAt: new Date().toISOString(),
    boardEnv: result.boardEnv,
    compare: result.compare,
    release: {
      tag_name: result.release.tag_name,
      name: result.release.name,
      prerelease: result.release.prerelease,
      published_at: result.release.published_at,
      draft: result.release.draft,
    },
    firmwareAsset: result.firmwareAsset,
    checksumsAsset: result.checksumsAsset,
  };
}

export function storedToReleaseCheck(
  stored: StoredReleaseCheckV1,
  channel: ReleaseChannel,
  currentVersion: string,
  chipModel: string | undefined,
): ReleaseCheckResult | null {
  const boardEnv = pickBoardEnv(chipModel);
  if (stored.channel !== channel || stored.boardEnv !== boardEnv) return null;
  const release = releaseFromStored(stored);
  return {
    release,
    channel: stored.channel,
    boardEnv: stored.boardEnv,
    firmwareAsset: stored.firmwareAsset,
    checksumsAsset: stored.checksumsAsset,
    compare: compareFirmwareVersions(currentVersion, release.tag_name),
  };
}

function parseStoredReleaseCheck(raw: string): StoredReleaseCheckV1 | null {
  try {
    const parsed = JSON.parse(raw) as StoredReleaseCheckV1;
    if (parsed?.v !== 1) return null;
    if (!parsed.firmwareAsset?.url || !parsed.checksumsAsset?.url) return null;
    return parsed;
  } catch {
    return null;
  }
}

function importReleaseCheckFromSession(channel: ReleaseChannel): StoredReleaseCheckV1 | null {
  try {
    const sessionPayload =
      sessionStorage.getItem(FW_CHECK_STORAGE_KEY) ??
      localStorage.getItem(FW_CHECK_STORAGE_KEY);
    if (!sessionPayload) return null;
    const parsed = parseStoredReleaseCheck(sessionPayload);
    if (!parsed || parsed.channel !== channel) return null;
    localStorage.setItem(checkStorageKey(channel), sessionPayload);
    sessionStorage.removeItem(FW_CHECK_STORAGE_KEY);
    localStorage.removeItem(FW_CHECK_STORAGE_KEY);
    return parsed;
  } catch {
    return null;
  }
}

export function storeReleaseCheck(result: ReleaseCheckResult): void {
  try {
    localStorage.setItem(
      checkStorageKey(result.channel),
      JSON.stringify(releaseCheckToStored(result)),
    );
  } catch {
    /* ignore */
  }
}

export function loadStoredReleaseCheck(
  channel: ReleaseChannel,
): StoredReleaseCheckV1 | null {
  try {
    const raw = localStorage.getItem(checkStorageKey(channel));
    if (raw) return parseStoredReleaseCheck(raw);
    return importReleaseCheckFromSession(channel);
  } catch {
    return null;
  }
}

export function clearStoredReleaseCheck(channel?: ReleaseChannel): void {
  try {
    if (channel) {
      localStorage.removeItem(checkStorageKey(channel));
      return;
    }
    localStorage.removeItem(checkStorageKey("stable"));
    localStorage.removeItem(checkStorageKey("prerelease"));
    sessionStorage.removeItem(FW_CHECK_STORAGE_KEY);
    localStorage.removeItem(FW_CHECK_STORAGE_KEY);
  } catch {
    /* ignore */
  }
}

interface StoredAutoCheckV1 {
  v: 1;
  stable?: string;
  prerelease?: string;
}

function loadAutoCheckStore(): StoredAutoCheckV1 {
  try {
    const raw = localStorage.getItem(FW_AUTO_CHECK_STORAGE_KEY);
    if (!raw) return { v: 1 };
    const parsed = JSON.parse(raw) as StoredAutoCheckV1;
    if (parsed?.v === 1) return parsed;
  } catch {
    /* ignore */
  }
  return { v: 1 };
}

function saveAutoCheckStore(store: StoredAutoCheckV1): void {
  try {
    localStorage.setItem(FW_AUTO_CHECK_STORAGE_KEY, JSON.stringify(store));
  } catch {
    /* ignore */
  }
}

/** True when this channel has not had an automatic GitHub check in the last 24 h. */
export function shouldRunDailyAutoCheck(
  channel: ReleaseChannel,
  nowMs: number = Date.now(),
): boolean {
  const store = loadAutoCheckStore();
  const last = store[channel];
  if (!last) return true;
  const lastMs = Date.parse(last);
  if (!Number.isFinite(lastMs)) return true;
  return nowMs - lastMs >= MS_PER_DAY;
}

export function markDailyAutoCheck(
  channel: ReleaseChannel,
  at: Date = new Date(),
): void {
  const store = loadAutoCheckStore();
  store[channel] = at.toISOString();
  saveAutoCheckStore(store);
}

export async function fetchFirmwareChecksum(
  release: GhRelease,
  firmwareAsset: GhAsset,
): Promise<{ checksumsAsset: GhAsset; firmwareSha256: string }> {
  const checksumsAsset = findChecksumsAsset(release);
  if (!checksumsAsset) throw new GithubReleaseError("checksums_missing");
  const sumsBuf = await downloadReleaseAsset(checksumsAsset);
  const sumsText = new TextDecoder().decode(sumsBuf);
  const firmwareSha256 = parseSha256ForFile(sumsText, firmwareAsset.name);
  if (!firmwareSha256) throw new GithubReleaseError("checksum_line_missing");
  return { checksumsAsset, firmwareSha256 };
}

export async function checkGithubRelease(
  channel: ReleaseChannel,
  currentVersion: string,
  chipModel: string | undefined,
): Promise<ReleaseCheckResult> {
  const release = await fetchReleaseForChannel(channel);
  const boardEnv = pickBoardEnv(chipModel);
  const firmwareAsset = findFirmwareAsset(release, boardEnv);
  const checksumsAsset = findChecksumsAsset(release);
  if (!checksumsAsset) throw new GithubReleaseError("checksums_missing");
  const compare = compareFirmwareVersions(currentVersion, release.tag_name);
  return {
    release,
    channel,
    boardEnv,
    firmwareAsset,
    checksumsAsset,
    compare,
  };
}

export async function downloadAndVerifyFirmware(
  release: GhRelease,
  firmwareAsset: GhAsset,
): Promise<ArrayBuffer> {
  const sumsAsset = findChecksumsAsset(release);
  if (!sumsAsset) throw new GithubReleaseError("checksums_missing");

  const [firmwareBuf, sumsBuf] = await Promise.all([
    downloadReleaseAsset(firmwareAsset),
    downloadReleaseAsset(sumsAsset),
  ]);
  const sumsText = new TextDecoder().decode(sumsBuf);
  await verifyFirmwareSha256(firmwareBuf, sumsText, firmwareAsset.name);
  return firmwareBuf;
}
