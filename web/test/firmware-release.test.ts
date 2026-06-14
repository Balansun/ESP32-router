import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { pickBoardEnv } from "../src/firmware/boardEnv";
import {
  assetDownloadUrlForDevice,
  clearStoredReleaseCheck,
  downloadReleaseAsset,
  checkGithubRelease,
  fetchReleaseForChannel,
  findFirmwareAsset,
  findChecksumsAsset,
  fetchFirmwareChecksum,
  firmwareAssetName,
  GithubReleaseError,
  githubApiHeaders,
  FW_CHANNEL_STORAGE_KEY,
  FW_CHECK_STORAGE_KEY,
  FW_AUTO_CHECK_STORAGE_KEY,
  isProductLineReleaseTag,
  loadStoredChannel,
  loadStoredReleaseCheck,
  markDailyAutoCheck,
  parseSha256ForFile,
  releaseCheckToStored,
  resolveRelease,
  sha256Hex,
  shouldRunDailyAutoCheck,
  storeChannel,
  storeReleaseCheck,
  storedToReleaseCheck,
  verifyFirmwareSha256,
  downloadAndVerifyFirmware,
  type GhAsset,
  type GhRelease,
} from "../src/firmware/githubRelease";
import {
  compareFirmwareVersions,
  formatFirmwareVersionFull,
  isSameReleaseTag,
  normalizeTagForCompare,
  parseVersionParts,
} from "../src/firmware/versionCompare";

const FIXTURE_RELEASES: GhRelease[] = [
  {
    tag_name: "v0.2.0-rc.1",
    name: "0.2.0-rc.1",
    prerelease: true,
    draft: false,
    published_at: "2026-05-01T00:00:00Z",
    assets: [
      {
        name: "balansun-0.2.0-rc.1-wroom32-firmware.bin",
        size: 100,
        browser_download_url: "https://example.com/rc.bin",
        url: "https://api.github.com/assets/1",
      },
      {
        name: "SHA256SUMS.txt",
        size: 80,
        browser_download_url: "https://example.com/sums.txt",
        url: "https://api.github.com/assets/2",
      },
    ],
  },
  {
    tag_name: "v0.2.0",
    name: "0.2.0",
    prerelease: false,
    draft: false,
    published_at: "2026-04-01T00:00:00Z",
    assets: [
      {
        name: "balansun-0.2.0-wroom32-firmware.bin",
        size: 200,
        browser_download_url: "https://example.com/stable.bin",
        url: "https://api.github.com/assets/3",
      },
      {
        name: "balansun-0.2.0-esp32s3-firmware.bin",
        size: 210,
        browser_download_url: "https://example.com/s3.bin",
        url: "https://api.github.com/assets/4",
      },
      {
        name: "SHA256SUMS.txt",
        size: 120,
        browser_download_url: "https://example.com/sums2.txt",
        url: "https://api.github.com/assets/5",
      },
    ],
  },
];

describe("versionCompare", () => {
  it("formats full device version for display", () => {
    expect(formatFirmwareVersionFull("0.1.0")).toBe("0.1.0");
    expect(formatFirmwareVersionFull("6.10_RMS")).toBe("6.10_RMS");
    expect(formatFirmwareVersionFull("  ")).toBe("—");
  });

  it("parses device and tag versions", () => {
    expect(parseVersionParts("0.1.0")).toEqual([0, 1, 0]);
    expect(parseVersionParts("6.10_RMS")).toEqual([6, 10]);
    expect(parseVersionParts("v0.2.0")).toEqual([0, 2, 0]);
  });

  it("normalizes tags for same-release detection", () => {
    expect(normalizeTagForCompare("v0.1.0-rc.2")).toBe("0.1.0-rc.2");
    expect(isSameReleaseTag("0.1.0-rc.2", "v0.1.0-rc.2")).toBe(true);
    expect(isSameReleaseTag("0.1.0-rc.2", "v0.1.0")).toBe(false);
  });

  it("compares current vs release", () => {
    expect(compareFirmwareVersions("0.1.0", "v0.2.0")).toBe(-1);
    expect(compareFirmwareVersions("0.2.0", "v0.2.0")).toBe(0);
    expect(compareFirmwareVersions("0.1.0-rc.2", "v0.1.0-rc.2")).toBe(0);
    expect(compareFirmwareVersions("0.3.0", "v0.2.0")).toBe(1);
    expect(compareFirmwareVersions("6.10_RMS", "v0.2.0")).toBe(1);
  });
});

describe("boardEnv", () => {
  it("maps chip model to PlatformIO env", () => {
    expect(pickBoardEnv("ESP32-D0WD-V3")).toBe("wroom32");
    expect(pickBoardEnv("ESP32-S3")).toBe("esp32s3");
    expect(pickBoardEnv(undefined)).toBe("wroom32");
  });
});

describe("githubRelease", () => {
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
  });

  afterEach(() => {
    vi.unstubAllGlobals();
  });

  it("uses default owner and repo when env vars are blank", async () => {
    vi.stubEnv("VITE_GITHUB_OWNER", "  ");
    vi.stubEnv("VITE_GITHUB_REPO", "");
    vi.resetModules();
    const mod = await import("../src/firmware/githubRelease");
    expect(mod.GITHUB_OWNER).toBe("Balansun");
    expect(mod.GITHUB_REPO).toBe("ESP32-router");
    expect(mod.GITHUB_RELEASES_PAGE).toContain("Balansun/ESP32-router");
    vi.unstubAllEnvs();
    vi.resetModules();
  });

  it("identifies 0.x product-line tags", () => {
    expect(isProductLineReleaseTag("v0.1.0")).toBe(true);
    expect(isProductLineReleaseTag("v6.11.0")).toBe(false);
  });

  it("builds GitHub API headers without Authorization", () => {
    expect(githubApiHeaders()).toMatchObject({
      Accept: "application/vnd.github+json",
      "X-GitHub-Api-Version": "2022-11-28",
    });
    expect(githubApiHeaders().Authorization).toBeUndefined();
    expect(githubApiHeaders({ "X-Custom": "1" })["X-Custom"]).toBe("1");
  });

  it("resolves stable and prerelease channels", () => {
    expect(resolveRelease("stable", FIXTURE_RELEASES).tag_name).toBe("v0.2.0");
    expect(resolveRelease("prerelease", FIXTURE_RELEASES).tag_name).toBe(
      "v0.2.0-rc.1",
    );
  });

  it("throws when no stable release exists", () => {
    const preOnly = FIXTURE_RELEASES.filter((r) => r.prerelease);
    expect(() => resolveRelease("stable", preOnly)).toThrow();
  });

  it("throws when no prerelease exists", () => {
    const stableOnly = FIXTURE_RELEASES.filter((r) => !r.prerelease);
    expect(() => resolveRelease("prerelease", stableOnly)).toThrow();
  });

  it("finds firmware asset by board env", () => {
    const stable = resolveRelease("stable", FIXTURE_RELEASES);
    expect(
      findFirmwareAsset(stable, "wroom32").name,
    ).toBe("balansun-0.2.0-wroom32-firmware.bin");
    expect(
      findFirmwareAsset(stable, "esp32s3").name,
    ).toBe("balansun-0.2.0-esp32s3-firmware.bin");
  });

  it("builds expected asset filename", () => {
    expect(firmwareAssetName("0.2.0", "wroom32")).toBe(
      "balansun-0.2.0-wroom32-firmware.bin",
    );
  });

  it("findFirmwareAsset throws when asset missing", () => {
    const stable = resolveRelease("stable", FIXTURE_RELEASES);
    const noAssets = { ...stable, assets: [] };
    expect(() => findFirmwareAsset(noAssets, "wroom32")).toThrow();
  });

  it("parses SHA256SUMS line for firmware file", () => {
    const hash = "a".repeat(64);
    const sums = `${hash}  balansun-0.2.0-wroom32-firmware.bin`;
    expect(
      parseSha256ForFile(sums, "balansun-0.2.0-wroom32-firmware.bin"),
    ).toBe(hash);
    expect(parseSha256ForFile(sums, "other.bin")).toBeNull();
  });

  it("assetDownloadUrlForDevice prefers API url over browser_download_url", () => {
    const asset: GhAsset = {
      name: "balansun-0.2.0-wroom32-firmware.bin",
      size: 100,
      url: "https://api.github.com/repos/o/r/releases/assets/99",
      browser_download_url:
        "https://github.com/o/r/releases/download/v0.2.0/balansun-0.2.0-wroom32-firmware.bin",
    };
    expect(assetDownloadUrlForDevice(asset)).toBe(asset.url);
    expect(
      assetDownloadUrlForDevice({ ...asset, url: "", browser_download_url: asset.browser_download_url }),
    ).toBe(asset.browser_download_url);
  });

  it("downloads release assets via GitHub API URL (not browser_download_url)", async () => {
    const payload = new Uint8Array([0xde, 0xad, 0xbe, 0xef]);
    const asset: GhAsset = {
      name: "balansun-0.2.0-wroom32-firmware.bin",
      size: payload.length,
      browser_download_url: "https://github.com/example/releases/download/v1/fw.bin",
      url: "https://api.github.com/repos/o/r/releases/assets/99",
    };
    const fetchMock = vi.fn(async (input: RequestInfo | URL, init?: RequestInit) => {
      const url = String(input);
      if (url === asset.url) {
        expect(init?.headers).toMatchObject({
          Accept: "application/octet-stream",
        });
        return new Response(null, {
          status: 302,
          headers: { Location: "https://objects.githubusercontent.com/fw.bin" },
        });
      }
      expect(url).toBe("https://objects.githubusercontent.com/fw.bin");
      expect(init?.headers).toBeUndefined();
      return new Response(payload, { status: 200 });
    });
    vi.stubGlobal("fetch", fetchMock);

    const buf = await downloadReleaseAsset(asset);
    expect(new Uint8Array(buf)).toEqual(payload);
    expect(fetchMock).toHaveBeenCalledTimes(2);
  });

  it("loadStoredChannel defaults and stores", () => {
    expect(loadStoredChannel()).toBe("prerelease");
    storeChannel("prerelease");
    expect(loadStoredChannel()).toBe("prerelease");
    storeChannel("stable");
    expect(loadStoredChannel()).toBe("prerelease");
  });

  it("loadStoredChannel prefers localStorage when sessionStorage is blocked", () => {
    vi.stubGlobal("sessionStorage", {
      getItem: () => {
        throw new Error("blocked");
      },
      setItem: () => {
        throw new Error("blocked");
      },
    });
    expect(loadStoredChannel()).toBe("prerelease");
    storeChannel("prerelease");
    expect(loadStoredChannel()).toBe("prerelease");
    storeChannel("stable");
    expect(loadStoredChannel()).toBe("prerelease");
  });

  it("fetchReleaseForChannel maps API failures", async () => {
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValue({ ok: false, status: 403 }),
    );
    await expect(fetchReleaseForChannel("stable")).rejects.toMatchObject({
      code: "rate_limit",
    });
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValue({ ok: false, status: 404 }),
    );
    await expect(fetchReleaseForChannel("stable")).rejects.toMatchObject({
      code: "repo_not_found",
    });
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValue({ ok: false, status: 500 }),
    );
    await expect(fetchReleaseForChannel("stable")).rejects.toMatchObject({
      code: "github_fetch_failed",
    });
    vi.unstubAllGlobals();
  });

  it("sha256Hex and verifyFirmwareSha256", async () => {
    const buf = new TextEncoder().encode("firmware").buffer;
    const hash = await sha256Hex(buf);
    expect(hash).toHaveLength(64);
    const sums = `${hash}  fw.bin`;
    await verifyFirmwareSha256(buf, sums, "fw.bin");
    await expect(verifyFirmwareSha256(buf, sums, "other.bin")).rejects.toMatchObject({
      code: "checksum_line_missing",
    });
  });

  it("fetchReleaseForChannel and checkGithubRelease", async () => {
    vi.stubGlobal(
      "fetch",
      vi.fn(async (input: RequestInfo | URL) => {
        const url = String(input);
        if (url.includes("/releases")) {
          return { ok: true, json: async () => FIXTURE_RELEASES };
        }
        throw new Error(`unexpected fetch: ${url}`);
      }),
    );
    const rel = await fetchReleaseForChannel("stable");
    expect(rel.tag_name).toBe("v0.2.0");
    const check = await checkGithubRelease("stable", "0.1.0", "ESP32");
    expect(check.compare).toBe(-1);
    expect(check.checksumsAsset.name).toBe("SHA256SUMS.txt");
    expect(check.firmwareAsset.url).toContain("api.github.com");
    vi.unstubAllGlobals();
  });

  it("downloadAndVerifyFirmware verifies checksums", async () => {
    const payload = new Uint8Array([1, 2, 3]);
    const stable = resolveRelease("stable", FIXTURE_RELEASES);
    const asset = findFirmwareAsset(stable, "wroom32");
    const hash = await sha256Hex(payload.buffer);
    const sums = `${hash}  ${asset.name}\n`;
    let n = 0;
    const fetchMock = vi.fn(async () => {
      n++;
      if (n === 1) return new Response(payload, { status: 200 });
      return new Response(sums, { status: 200 });
    });
    vi.stubGlobal("fetch", fetchMock);
    expect(findChecksumsAsset(stable)?.name).toBe("SHA256SUMS.txt");
    const buf = await downloadAndVerifyFirmware(stable, asset);
    expect(new Uint8Array(buf)).toEqual(payload);
    vi.unstubAllGlobals();
  });

  it("parseSha256ForFile skips malformed lines", () => {
    expect(parseSha256ForFile("not-a-hash  file.bin", "file.bin")).toBeNull();
    expect(parseSha256ForFile("\n\n", "file.bin")).toBeNull();
  });

  it("parses checksum line ending with subdirectory path", () => {
    const hash = "a".repeat(64);
    expect(parseSha256ForFile(`${hash}  dist/fw.bin`, "fw.bin")).toBe(hash);
  });

  it("downloadReleaseAsset treats 403 as rate limit", async () => {
    const asset: GhAsset = {
      name: "x.bin",
      size: 1,
      browser_download_url: "",
      url: "https://api.github.com/asset/1",
    };
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValue({
        ok: false,
        status: 403,
        headers: new Headers(),
      }),
    );
    await expect(downloadReleaseAsset(asset)).rejects.toMatchObject({ code: "rate_limit" });
    vi.unstubAllGlobals();
  });

  it("downloadReleaseAsset rejects non-success HTTP status", async () => {
    const asset: GhAsset = {
      name: "x.bin",
      size: 1,
      browser_download_url: "",
      url: "https://api.github.com/asset/1",
    };
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValue({
        ok: false,
        status: 500,
        headers: new Headers(),
      }),
    );
    await expect(downloadReleaseAsset(asset)).rejects.toMatchObject({
      code: "download_failed",
    });
    vi.unstubAllGlobals();
  });

  it("downloadReleaseAsset rejects when blob download fails after redirect", async () => {
    const asset: GhAsset = {
      name: "x.bin",
      size: 1,
      browser_download_url: "",
      url: "https://api.github.com/asset/1",
    };
    vi.stubGlobal(
      "fetch",
      vi.fn(async (input: RequestInfo | URL) => {
        const url = String(input);
        if (url === asset.url) {
          return new Response(null, {
            status: 302,
            headers: { Location: "https://objects.example/fw.bin" },
          });
        }
        return new Response(null, { status: 503 });
      }),
    );
    await expect(downloadReleaseAsset(asset)).rejects.toMatchObject({
      code: "download_failed",
    });
    vi.unstubAllGlobals();
  });

  it("downloadReleaseAsset rejects redirect without Location", async () => {
    const asset: GhAsset = {
      name: "x.bin",
      size: 1,
      browser_download_url: "",
      url: "https://api.github.com/asset/1",
    };
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValue({
        ok: false,
        status: 302,
        headers: new Headers(),
      }),
    );
    await expect(downloadReleaseAsset(asset)).rejects.toMatchObject({
      code: "download_failed",
    });
    vi.unstubAllGlobals();
  });

  it("downloadAndVerifyFirmware requires checksum asset", async () => {
    const stable = FIXTURE_RELEASES.find((r) => r.tag_name === "v0.2.0-rc.1")!;
    const noSums: GhRelease = {
      ...stable,
      assets: stable.assets.filter((a) => a.name !== "SHA256SUMS.txt"),
    };
    await expect(downloadAndVerifyFirmware(noSums, noSums.assets[0])).rejects.toMatchObject({
      code: "checksums_missing",
    });
  });

  it("verifyFirmwareSha256 rejects hash mismatch", async () => {
    const buf = new TextEncoder().encode("x").buffer;
    const sums = `${"b".repeat(64)}  match.bin`;
    await expect(verifyFirmwareSha256(buf, sums, "match.bin")).rejects.toMatchObject({
      code: "checksum_mismatch",
    });
  });

  it("maps missing API asset URL to download_failed", async () => {
    const asset: GhAsset = {
      name: "x.bin",
      size: 1,
      browser_download_url: "https://example.com/x.bin",
      url: "",
    };
    await expect(downloadReleaseAsset(asset)).rejects.toMatchObject({
      code: "download_failed",
    } satisfies Partial<GithubReleaseError>);
  });

  it("stores and restores release check with checksum and asset URLs", async () => {
    localStorage.removeItem(`${FW_CHECK_STORAGE_KEY}:stable`);
    localStorage.removeItem(`${FW_CHECK_STORAGE_KEY}:prerelease`);
    const stable = resolveRelease("stable", FIXTURE_RELEASES);
    const firmwareAsset = findFirmwareAsset(stable, "wroom32");
    const checksumsAsset = findChecksumsAsset(stable)!;
    const result = {
      release: stable,
      channel: "stable" as const,
      boardEnv: "wroom32" as const,
      firmwareAsset,
      checksumsAsset,
      compare: -1 as const,
    };
    storeReleaseCheck(result);
    const stored = loadStoredReleaseCheck("stable");
    expect(stored?.firmwareAsset.url).toBe(firmwareAsset.url);
    expect(stored?.checksumsAsset.url).toBe(checksumsAsset.url);

    const hydrated = storedToReleaseCheck(stored!, "stable", "0.1.0", "ESP32");
    expect(hydrated?.release.tag_name).toBe("v0.2.0");
    expect(hydrated?.compare).toBe(-1);
    expect(hydrated?.checksumsAsset.url).toBe(checksumsAsset.url);

    clearStoredReleaseCheck("stable");
    expect(loadStoredReleaseCheck("stable")).toBeNull();
  });

  it("stores release checks per channel independently", () => {
    const stable = resolveRelease("stable", FIXTURE_RELEASES);
    const prerelease = resolveRelease("prerelease", FIXTURE_RELEASES);
    const stableAsset = findFirmwareAsset(stable, "wroom32");
    const prereleaseAsset = findFirmwareAsset(prerelease, "wroom32");
    const stableSums = findChecksumsAsset(stable)!;
    const prereleaseSums = findChecksumsAsset(prerelease)!;
    storeReleaseCheck({
      release: stable,
      channel: "stable",
      boardEnv: "wroom32",
      firmwareAsset: stableAsset,
      checksumsAsset: stableSums,
      compare: -1,
    });
    storeReleaseCheck({
      release: prerelease,
      channel: "prerelease",
      boardEnv: "wroom32",
      firmwareAsset: prereleaseAsset,
      checksumsAsset: prereleaseSums,
      compare: -1,
    });
    expect(loadStoredReleaseCheck("stable")?.release.tag_name).toBe("v0.2.0");
    expect(loadStoredReleaseCheck("prerelease")?.release.tag_name).toBe("v0.2.0-rc.1");
    clearStoredReleaseCheck();
  });

  it("shouldRunDailyAutoCheck respects 24 h window per channel", () => {
    localStorage.removeItem(FW_AUTO_CHECK_STORAGE_KEY);
    const now = Date.parse("2026-05-20T12:00:00.000Z");
    expect(shouldRunDailyAutoCheck("stable", now)).toBe(true);
    markDailyAutoCheck("stable", new Date(now));
    expect(shouldRunDailyAutoCheck("stable", now + 1000)).toBe(false);
    expect(shouldRunDailyAutoCheck("prerelease", now + 1000)).toBe(true);
    expect(shouldRunDailyAutoCheck("stable", now + 24 * 60 * 60 * 1000)).toBe(true);
  });

  it("shouldRunDailyAutoCheck treats corrupt store as never checked", () => {
    localStorage.setItem(FW_AUTO_CHECK_STORAGE_KEY, "not-json");
    expect(shouldRunDailyAutoCheck("stable")).toBe(true);
    localStorage.setItem(FW_AUTO_CHECK_STORAGE_KEY, JSON.stringify({ v: 2 }));
    expect(shouldRunDailyAutoCheck("prerelease")).toBe(true);
    localStorage.setItem(
      FW_AUTO_CHECK_STORAGE_KEY,
      JSON.stringify({ v: 1, stable: "not-a-date" }),
    );
    expect(shouldRunDailyAutoCheck("stable")).toBe(true);
  });

  it("markDailyAutoCheck ignores storage write failures", () => {
    const storage = {
      getItem: () => null,
      setItem: () => {
        throw new Error("quota");
      },
    } as Storage;
    vi.stubGlobal("localStorage", storage);
    expect(() => markDailyAutoCheck("stable", new Date())).not.toThrow();
    vi.unstubAllGlobals();
  });

  it("fetchFirmwareChecksum downloads sums and parses firmware hash", async () => {
    const stable = resolveRelease("stable", FIXTURE_RELEASES);
    const firmwareAsset = findFirmwareAsset(stable, "wroom32");
    const hash = "b".repeat(64);
    const sumsText = `${hash}  ${firmwareAsset.name}\n`;
    vi.stubGlobal(
      "fetch",
      vi.fn(async (input: RequestInfo | URL) => {
        const url = String(input);
        const sumsAsset = findChecksumsAsset(stable)!;
        if (url === sumsAsset.url) {
          return new Response(new TextEncoder().encode(sumsText), { status: 200 });
        }
        return new Response(null, { status: 404 });
      }),
    );
    const result = await fetchFirmwareChecksum(stable, firmwareAsset);
    expect(result.firmwareSha256).toBe(hash);
    expect(result.checksumsAsset.name).toBe("SHA256SUMS.txt");
    vi.unstubAllGlobals();
  });

  it("fetchFirmwareChecksum maps missing checksums asset", async () => {
    const stable = resolveRelease("stable", FIXTURE_RELEASES);
    const firmwareAsset = findFirmwareAsset(stable, "wroom32");
    const noSums: GhRelease = {
      ...stable,
      assets: stable.assets.filter((a) => a.name !== "SHA256SUMS.txt"),
    };
    await expect(fetchFirmwareChecksum(noSums, firmwareAsset)).rejects.toMatchObject({
      code: "checksums_missing",
    });
  });

  it("fetchFirmwareChecksum maps missing firmware line in sums", async () => {
    const stable = resolveRelease("stable", FIXTURE_RELEASES);
    const firmwareAsset = findFirmwareAsset(stable, "wroom32");
    vi.stubGlobal(
      "fetch",
      vi.fn(async () => new Response(new TextEncoder().encode("deadbeef\n"), { status: 200 })),
    );
    await expect(fetchFirmwareChecksum(stable, firmwareAsset)).rejects.toMatchObject({
      code: "checksum_line_missing",
    });
    vi.unstubAllGlobals();
  });

  it("storeReleaseCheck and loadStoredReleaseCheck tolerate storage failures", () => {
    const throwing = {
      getItem: () => {
        throw new Error("blocked");
      },
      setItem: () => {
        throw new Error("blocked");
      },
      removeItem: () => {
        throw new Error("blocked");
      },
    } as Storage;
    vi.stubGlobal("localStorage", throwing);
    const stable = resolveRelease("stable", FIXTURE_RELEASES);
    const firmwareAsset = findFirmwareAsset(stable, "wroom32");
    const checksumsAsset = findChecksumsAsset(stable)!;
    expect(() =>
      storeReleaseCheck({
        release: stable,
        channel: "stable",
        boardEnv: "wroom32",
        firmwareAsset,
        checksumsAsset,
        compare: -1,
      }),
    ).not.toThrow();
    expect(loadStoredReleaseCheck("stable")).toBeNull();
    expect(() => clearStoredReleaseCheck()).not.toThrow();
    vi.unstubAllGlobals();
  });

  it("imports release check from sessionStorage", () => {
    const stable = resolveRelease("stable", FIXTURE_RELEASES);
    const firmwareAsset = findFirmwareAsset(stable, "wroom32");
    const checksumsAsset = findChecksumsAsset(stable)!;
    const sessionPayload = JSON.stringify(
      releaseCheckToStored({
        release: stable,
        channel: "stable",
        boardEnv: "wroom32",
        firmwareAsset,
        checksumsAsset,
        compare: -1,
      }),
    );
    vi.stubGlobal("sessionStorage", {
      getItem: (k: string) => (k === FW_CHECK_STORAGE_KEY ? sessionPayload : null),
      setItem: () => {},
      removeItem: () => {},
    });
    localStorage.removeItem(`${FW_CHECK_STORAGE_KEY}:stable`);
    const imported = loadStoredReleaseCheck("stable");
    expect(imported?.release.tag_name).toBe("v0.2.0");
    expect(lsBacking.get(`${FW_CHECK_STORAGE_KEY}:stable`)).toBe(sessionPayload);
  });

  it("session import tolerates storage write failures", () => {
    const stable = resolveRelease("stable", FIXTURE_RELEASES);
    const firmwareAsset = findFirmwareAsset(stable, "wroom32");
    const checksumsAsset = findChecksumsAsset(stable)!;
    const sessionPayload = JSON.stringify(
      releaseCheckToStored({
        release: stable,
        channel: "stable",
        boardEnv: "wroom32",
        firmwareAsset,
        checksumsAsset,
        compare: -1,
      }),
    );
    vi.stubGlobal("sessionStorage", {
      getItem: (k: string) => (k === FW_CHECK_STORAGE_KEY ? sessionPayload : null),
      setItem: () => {},
      removeItem: () => {},
    });
    vi.stubGlobal("localStorage", {
      getItem: () => null,
      setItem: () => {
        throw new Error("quota");
      },
      removeItem: () => {},
    });
    expect(loadStoredReleaseCheck("stable")).toBeNull();
    vi.unstubAllGlobals();
  });

  it("unscoped release check is ignored when channel mismatches", () => {
    const prerelease = resolveRelease("prerelease", FIXTURE_RELEASES);
    const firmwareAsset = findFirmwareAsset(prerelease, "wroom32");
    const checksumsAsset = findChecksumsAsset(prerelease)!;
    const storedPayload = JSON.stringify(
      releaseCheckToStored({
        release: prerelease,
        channel: "prerelease",
        boardEnv: "wroom32",
        firmwareAsset,
        checksumsAsset,
        compare: -1,
      }),
    );
    localStorage.setItem(FW_CHECK_STORAGE_KEY, storedPayload);
    expect(loadStoredReleaseCheck("stable")).toBeNull();
    localStorage.removeItem(FW_CHECK_STORAGE_KEY);
  });

  it("loadStoredReleaseCheck rejects corrupt stored JSON", () => {
    localStorage.setItem(`${FW_CHECK_STORAGE_KEY}:stable`, "{not-json");
    expect(loadStoredReleaseCheck("stable")).toBeNull();
    localStorage.setItem(
      `${FW_CHECK_STORAGE_KEY}:stable`,
      JSON.stringify({ v: 2, channel: "stable" }),
    );
    expect(loadStoredReleaseCheck("stable")).toBeNull();
    localStorage.removeItem(`${FW_CHECK_STORAGE_KEY}:stable`);
  });

  it("storeChannel ignores storage failures", () => {
    const throwing = {
      getItem: () => null,
      setItem: () => {
        throw new Error("quota");
      },
      removeItem: () => {
        throw new Error("quota");
      },
    } as Storage;
    vi.stubGlobal("localStorage", throwing);
    vi.stubGlobal("sessionStorage", throwing);
    expect(() => storeChannel("prerelease")).not.toThrow();
    vi.unstubAllGlobals();
  });

  it("loadStoredChannel maps legacy and alias values to nightly", () => {
    localStorage.setItem(FW_CHANNEL_STORAGE_KEY, "nightly");
    expect(loadStoredChannel()).toBe("prerelease");
    localStorage.setItem(FW_CHANNEL_STORAGE_KEY, "stable");
    expect(loadStoredChannel()).toBe("prerelease");
    localStorage.removeItem(FW_CHANNEL_STORAGE_KEY);
  });

  it("downloadReleaseAsset follows non-302 redirects", async () => {
    const payload = new Uint8Array([7, 8, 9]);
    const asset: GhAsset = {
      name: "x.bin",
      size: payload.length,
      browser_download_url: "",
      url: "https://api.github.com/asset/307",
    };
    vi.stubGlobal(
      "fetch",
      vi.fn(async (input: RequestInfo | URL) => {
        const url = String(input);
        if (url === asset.url) {
          return new Response(null, {
            status: 307,
            headers: { Location: "https://objects.example/x.bin" },
          });
        }
        return new Response(payload, { status: 200 });
      }),
    );
    const buf = await downloadReleaseAsset(asset);
    expect(new Uint8Array(buf)).toEqual(payload);
    vi.unstubAllGlobals();
  });

  it("storedToReleaseCheck rejects channel or board mismatch", () => {
    const stable = resolveRelease("stable", FIXTURE_RELEASES);
    const firmwareAsset = findFirmwareAsset(stable, "wroom32");
    const checksumsAsset = findChecksumsAsset(stable)!;
    const stored = releaseCheckToStored({
      release: stable,
      channel: "stable",
      boardEnv: "wroom32",
      firmwareAsset,
      checksumsAsset,
      compare: -1,
    });
    expect(storedToReleaseCheck(stored, "prerelease", "0.1.0", "ESP32")).toBeNull();
    expect(storedToReleaseCheck(stored, "stable", "0.1.0", "ESP32-S3")).toBeNull();
  });
});
