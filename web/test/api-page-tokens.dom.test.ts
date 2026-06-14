import { describe, expect, it, vi, beforeEach } from "vitest";
import { mountApi } from "../src/routes/Api";
import { en } from "../src/i18n/locales/en";
import { publicBootstrap } from "../src/state/store";
import { clearSession, setSessionToken } from "../src/api/apiSession";

const copyTextToClipboardSync = vi.hoisted(() => vi.fn(() => true));
vi.mock("../src/utils/copyToClipboard", () => ({
  copyTextToClipboardSync,
}));

const getConfig = vi.hoisted(() =>
  vi.fn(async () => ({ config: { http_cors_enabled: false } })),
);
const getPublic = vi.hoisted(() => vi.fn());
const listAuthTokens = vi.hoisted(() => vi.fn());
const createAuthToken = vi.hoisted(() => vi.fn());

vi.mock("../src/api/client", async (importOriginal) => {
  const orig = await importOriginal<typeof import("../src/api/client")>();
  return {
    ...orig,
    api: {
      ...orig.api,
      getConfig,
      getPublic,
      listAuthTokens,
      createAuthToken,
    },
  };
});

const samplePublic = (enabled: boolean) => ({
  http_auth: { enabled, username: "admin" },
  device: {
    router_name: "Balansun",
    firmware_version: "0.1.0",
    source_configured: true,
  },
  wifi: { mode: "sta" as const, connected: true },
});

describe("Api page permanent tokens", () => {
  beforeEach(() => {
    document.body.innerHTML = "";
    clearSession();
    publicBootstrap.set({ ready: false, httpAuthEnabled: false });
    getConfig.mockClear();
    listAuthTokens.mockReset();
    createAuthToken.mockReset();
    copyTextToClipboardSync.mockReset();
    copyTextToClipboardSync.mockReturnValue(true);
  });

  it("disables create token when HTTP API password is not enabled", async () => {
    getPublic.mockResolvedValue(samplePublic(false));
    const outlet = document.createElement("div");
    document.body.append(outlet);
    const cleanup = await mountApi({ outlet, signal: new AbortController().signal });
    await new Promise((r) => setTimeout(r, 0));

    const tokensCard = Array.from(outlet.querySelectorAll(".card")).find((c) =>
      c.textContent?.includes(en.apiPage.sectionTokens),
    );
    const btn = tokensCard?.querySelector(".btn--primary") as HTMLButtonElement;
    expect(btn?.textContent).toBe(en.apiPage.tokenCreate);
    expect(btn?.disabled).toBe(true);
    expect(
      tokensCard?.querySelector(".banner.banner--warn[role='alert']")?.hidden,
    ).toBe(false);
    cleanup();
  });

  it("disables create token when auth enabled but not signed in", async () => {
    getPublic.mockResolvedValue(samplePublic(true));
    listAuthTokens.mockResolvedValue([]);
    publicBootstrap.set({ ready: true, httpAuthEnabled: true });
    const outlet = document.createElement("div");
    document.body.append(outlet);
    const cleanup = await mountApi({ outlet, signal: new AbortController().signal });
    await new Promise((r) => setTimeout(r, 0));

    const tokensCard = Array.from(outlet.querySelectorAll(".card")).find((c) =>
      c.textContent?.includes(en.apiPage.sectionTokens),
    );
    const btn = tokensCard?.querySelector(".btn--primary") as HTMLButtonElement;
    expect(btn?.disabled).toBe(true);
    expect(tokensCard?.querySelector(".banner.banner--info")?.hidden).toBe(false);
    cleanup();
  });

  it("token dialog copy runs sync helper and closes on success", async () => {
    getPublic.mockResolvedValue(samplePublic(true));
    listAuthTokens.mockResolvedValue([]);
    createAuthToken.mockResolvedValue({
      token: "abc123token",
      label: "HA-test",
      id: 3,
    });
    setSessionToken("a".repeat(64));
    publicBootstrap.set({ ready: true, httpAuthEnabled: true });

    const outlet = document.createElement("div");
    document.body.append(outlet);
    const cleanup = await mountApi({ outlet, signal: new AbortController().signal });
    await new Promise((r) => setTimeout(r, 0));

    const tokensCard = Array.from(outlet.querySelectorAll(".card")).find((c) =>
      c.textContent?.includes(en.apiPage.sectionTokens),
    );
    const createBtn = tokensCard?.querySelector(".btn--primary") as HTMLButtonElement;
    createBtn.click();
    await new Promise((r) => setTimeout(r, 0));

    const dlg = document.querySelector("dialog.sheet-dialog");
    expect(dlg?.textContent).toContain(en.apiPage.tokenCopyManualHint);
    const tokenInput = dlg?.querySelector(
      "input.field__input[readonly]",
    ) as HTMLInputElement;
    expect(tokenInput?.value).toBe("abc123token");

    const copyBtn = dlg?.querySelector(".btn--primary") as HTMLButtonElement;
    copyBtn.click();
    expect(copyTextToClipboardSync).toHaveBeenCalledWith("abc123token", tokenInput);
    expect(dlg?.open).toBe(false);
    await new Promise((r) => setTimeout(r, 250));
    expect(document.querySelector("dialog.sheet-dialog")).toBeNull();

    cleanup();
    clearSession();
  });

  it("token dialog stays open when copy fails", async () => {
    copyTextToClipboardSync.mockReturnValue(false);
    getPublic.mockResolvedValue(samplePublic(true));
    listAuthTokens.mockResolvedValue([]);
    createAuthToken.mockResolvedValue({
      token: "fail-token",
      label: "x",
      id: 4,
    });
    setSessionToken("a".repeat(64));
    publicBootstrap.set({ ready: true, httpAuthEnabled: true });

    const outlet = document.createElement("div");
    document.body.append(outlet);
    const cleanup = await mountApi({ outlet, signal: new AbortController().signal });
    await new Promise((r) => setTimeout(r, 0));

    const tokensCard = Array.from(outlet.querySelectorAll(".card")).find((c) =>
      c.textContent?.includes(en.apiPage.sectionTokens),
    );
    (tokensCard?.querySelector(".btn--primary") as HTMLButtonElement).click();
    await new Promise((r) => setTimeout(r, 0));

    const dlg = document.querySelector("dialog.sheet-dialog");
    (dlg?.querySelector(".btn--primary") as HTMLButtonElement).click();
    expect(document.querySelector("dialog.sheet-dialog")).not.toBeNull();

    cleanup();
    clearSession();
  });
});
