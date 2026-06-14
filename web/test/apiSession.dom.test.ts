import { beforeEach, describe, expect, it, vi } from "vitest";
import {
  clearSession,
  getSessionAuthHeader,
  hasSession,
  logoutSession,
  probeApiSession,
  setSessionToken,
  verifyLoginPassword,
} from "../src/api/apiSession";

describe("apiSession", () => {
  beforeEach(() => {
    sessionStorage.clear();
  });

  it("getSessionAuthHeader returns Bearer format", () => {
    setSessionToken("a".repeat(64));
    expect(getSessionAuthHeader()).toBe(`Bearer ${"a".repeat(64)}`);
  });

  it("set, has, clear session", () => {
    setSessionToken("tok");
    expect(hasSession()).toBe(true);
    expect(getSessionAuthHeader()).toMatch(/^Bearer /);
    clearSession();
    expect(hasSession()).toBe(false);
    expect(getSessionAuthHeader()).toBeUndefined();
  });

  it("verifyLoginPassword stores token on success", async () => {
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValue({
        ok: true,
        status: 200,
        json: async () => ({ ok: true, token: "b".repeat(64) }),
      }),
    );
    expect(await verifyLoginPassword("good")).toBe(true);
    expect(hasSession()).toBe(true);
    vi.unstubAllGlobals();
  });

  it("verifyLoginPassword accepts ok without token when auth disabled", async () => {
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValue({
        ok: true,
        status: 200,
        json: async () => ({ ok: true }),
      }),
    );
    expect(await verifyLoginPassword("any")).toBe(true);
    expect(hasSession()).toBe(false);
    vi.unstubAllGlobals();
  });

  it("verifyLoginPassword rejects 401", async () => {
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValue({ ok: false, status: 401 }),
    );
    expect(await verifyLoginPassword("bad")).toBe(false);
    vi.unstubAllGlobals();
  });

  it("verifyLoginPassword rejects non-ok response and body.ok false", async () => {
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValueOnce({ ok: false, status: 500 }),
    );
    expect(await verifyLoginPassword("x")).toBe(false);
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValue({
        ok: true,
        status: 200,
        json: async () => ({ ok: false }),
      }),
    );
    expect(await verifyLoginPassword("x")).toBe(false);
    vi.unstubAllGlobals();
  });

  it("verifyLoginPassword handles network error", async () => {
    vi.stubGlobal("fetch", vi.fn().mockRejectedValue(new Error("offline")));
    expect(await verifyLoginPassword("x")).toBe(false);
    vi.unstubAllGlobals();
  });

  it("probeApiSession requires token and checks device", async () => {
    expect(await probeApiSession()).toBe(false);
    setSessionToken("d".repeat(64));
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValue({ ok: true, status: 200 }),
    );
    expect(await probeApiSession()).toBe(true);
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValue({ ok: false, status: 401 }),
    );
    expect(await probeApiSession()).toBe(false);
    vi.unstubAllGlobals();
  });

  it("probeApiSession handles network error", async () => {
    setSessionToken("e".repeat(64));
    vi.stubGlobal("fetch", vi.fn().mockRejectedValue(new Error("offline")));
    expect(await probeApiSession()).toBe(false);
    vi.unstubAllGlobals();
  });

  it("logoutSession clears local session", async () => {
    setSessionToken("c".repeat(64));
    const fetchMock = vi.fn().mockResolvedValue({ ok: true, status: 200 });
    vi.stubGlobal("fetch", fetchMock);
    await logoutSession();
    expect(hasSession()).toBe(false);
    expect(fetchMock).toHaveBeenCalledWith(
      "/api/v1/auth/logout",
      expect.objectContaining({ method: "POST" }),
    );
    vi.unstubAllGlobals();
  });

  it("logoutSession clears session without token and ignores fetch errors", async () => {
    const fetchMock = vi.fn();
    vi.stubGlobal("fetch", fetchMock);
    await logoutSession();
    expect(fetchMock).not.toHaveBeenCalled();
    setSessionToken("f".repeat(64));
    vi.stubGlobal("fetch", vi.fn().mockRejectedValue(new Error("offline")));
    await logoutSession();
    expect(hasSession()).toBe(false);
    vi.unstubAllGlobals();
  });

  it("tolerates sessionStorage failures on read", () => {
    vi.stubGlobal("sessionStorage", {
      getItem: () => {
        throw new Error("blocked");
      },
      setItem: () => {},
      removeItem: () => {},
    });
    expect(hasSession()).toBe(false);
    expect(getSessionAuthHeader()).toBeUndefined();
    vi.unstubAllGlobals();
  });

  it("tolerates sessionStorage failures on write", () => {
    vi.stubGlobal("sessionStorage", {
      getItem: () => null,
      setItem: () => {
        throw new Error("blocked");
      },
      removeItem: () => {
        throw new Error("blocked");
      },
    });
    setSessionToken("x");
    expect(hasSession()).toBe(false);
    clearSession();
    vi.unstubAllGlobals();
  });
});
