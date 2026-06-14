import { describe, expect, it, beforeEach, vi } from "vitest";

vi.hoisted(() => {
  const storage = new Map<string, string>();
  vi.stubGlobal("sessionStorage", {
    getItem: (k: string) => storage.get(k) ?? null,
    setItem: (k: string, v: string) => {
      storage.set(k, v);
    },
    removeItem: (k: string) => {
      storage.delete(k);
    },
    clear: () => storage.clear(),
    key: () => null,
    get length() {
      return storage.size;
    },
  });
});

import {
  clearSession,
  getSessionAuthHeader,
  setSessionToken,
} from "../src/api/apiSession";
import {
  consumeReturnTo,
  isAuthExemptPath,
  isLoginPath,
  LOGIN_PATH,
  needsHttpAuthLogin,
  needsHttpAuthLoginForPath,
  requiresHttpAuthSession,
  saveReturnTo,
} from "../src/auth/httpAuthGate";
import { publicBootstrap } from "../src/state/store";

describe("apiSession gate", () => {
  beforeEach(() => {
    clearSession();
  });

  it("getSessionAuthHeader uses Bearer after setSessionToken", () => {
    const token = "a".repeat(64);
    setSessionToken(token);
    expect(getSessionAuthHeader()).toBe(`Bearer ${token}`);
  });
});

describe("httpAuthGate", () => {
  beforeEach(() => {
    clearSession();
    publicBootstrap.set({ ready: false, httpAuthEnabled: false });
    vi.stubGlobal("location", { pathname: "/" });
  });

  it("LOGIN_PATH is /login", () => {
    expect(LOGIN_PATH).toBe("/login");
    expect(isLoginPath("/login")).toBe(true);
    expect(isLoginPath("/settings")).toBe(false);
  });

  it("isAuthExemptPath is true for /wifi only (not /wifi/station)", () => {
    expect(isAuthExemptPath("/wifi")).toBe(true);
    expect(isAuthExemptPath("/wifi/")).toBe(true);
    expect(isAuthExemptPath("/wifi/station")).toBe(false);
    expect(isAuthExemptPath("/")).toBe(false);
    expect(isAuthExemptPath("/settings")).toBe(false);
  });

  it("needsHttpAuthLogin when bootstrap ready, auth on, no session", () => {
    publicBootstrap.set({ ready: true, httpAuthEnabled: true });
    expect(needsHttpAuthLogin()).toBe(true);
    expect(needsHttpAuthLoginForPath("/settings")).toBe(true);
    setSessionToken("d".repeat(64));
    expect(needsHttpAuthLogin()).toBe(false);
    expect(needsHttpAuthLoginForPath("/settings")).toBe(false);
  });

  it("needsHttpAuthLoginForPath is false for /wifi and /login", () => {
    publicBootstrap.set({ ready: true, httpAuthEnabled: true });
    expect(needsHttpAuthLoginForPath("/wifi")).toBe(false);
    expect(needsHttpAuthLoginForPath("/login")).toBe(false);
  });

  it("needsHttpAuthLoginForPath is true for /wifi/station when auth on and no session", () => {
    publicBootstrap.set({ ready: true, httpAuthEnabled: true });
    expect(needsHttpAuthLoginForPath("/wifi/station")).toBe(true);
  });

  it("requiresHttpAuthSession is true on /login when auth on and no token", () => {
    publicBootstrap.set({ ready: true, httpAuthEnabled: true });
    vi.stubGlobal("location", { pathname: "/login" });
    expect(requiresHttpAuthSession()).toBe(true);
    expect(needsHttpAuthLogin()).toBe(false);
  });

  it("needsHttpAuthLogin is false on exempt /wifi path", () => {
    publicBootstrap.set({ ready: true, httpAuthEnabled: true });
    vi.stubGlobal("location", { pathname: "/wifi" });
    expect(needsHttpAuthLogin()).toBe(false);
  });

  it("saveReturnTo skips /login and consumeReturnTo ignores stored login path", () => {
    saveReturnTo("/login");
    saveReturnTo("/settings");
    expect(consumeReturnTo()).toBe("/settings");
  });

  it("saveReturnTo and consumeReturnTo round-trip", () => {
    saveReturnTo("/settings");
    expect(consumeReturnTo()).toBe("/settings");
    expect(consumeReturnTo()).toBe("/");
  });
});
