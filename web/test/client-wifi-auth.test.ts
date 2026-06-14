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

import * as httpAuthGate from "../src/auth/httpAuthGate";
import { api, ApiError } from "../src/api/client";
import { clearSession, setSessionToken } from "../src/api/apiSession";

function authHeader(init?: RequestInit): string | undefined {
  const h = init?.headers;
  if (!h || typeof h !== "object" || Array.isArray(h)) return undefined;
  const rec = h as Record<string, string>;
  return rec.Authorization ?? rec.authorization;
}

describe("api client Wi‑Fi auth", () => {
  beforeEach(() => {
    clearSession();
    vi.spyOn(httpAuthGate, "requestHttpAuthLogin").mockResolvedValue(false);
    vi.stubGlobal("location", { pathname: "/wifi" });
    vi.stubGlobal(
      "fetch",
      vi.fn(async (_url: string, init?: RequestInit) => {
        return new Response(
          JSON.stringify({
            ssid: "Home",
            mode: "sta",
            connected: true,
            rssi: -50,
            ip: "192.168.1.10",
          }),
          { status: 200, headers: { "Content-Type": "application/json" } },
        );
      }),
    );
  });

  it("sends Bearer when session exists and omitAuth is false", async () => {
    const token = "a".repeat(64);
    setSessionToken(token);
    await api.getWifi();
    const init = vi.mocked(fetch).mock.calls[0]?.[1];
    expect(authHeader(init)).toBe(`Bearer ${token}`);
  });

  it("omits Bearer when omitAuth is true", async () => {
    setSessionToken("b".repeat(64));
    await api.getWifi({ omitAuth: true });
    const init = vi.mocked(fetch).mock.calls[0]?.[1];
    expect(authHeader(init)).toBeUndefined();
  });

  it("does not open login gate on 401 while on /wifi", async () => {
    vi.mocked(fetch).mockResolvedValueOnce(
      new Response(JSON.stringify({ error: "unauthorized" }), {
        status: 401,
        headers: { "Content-Type": "application/json" },
      }),
    );
    await expect(api.getWifi({ omitAuth: true })).rejects.toBeInstanceOf(ApiError);
    expect(httpAuthGate.requestHttpAuthLogin).not.toHaveBeenCalled();
  });

  it("opens login gate on 401 on protected routes", async () => {
    vi.stubGlobal("location", { pathname: "/" });
    vi.stubGlobal(
      "fetch",
      vi.fn(async () =>
        new Response(JSON.stringify({ error: "unauthorized" }), {
          status: 401,
          headers: { "Content-Type": "application/json" },
        }),
      ),
    );
    await expect(api.getHealth()).rejects.toBeInstanceOf(ApiError);
    expect(httpAuthGate.requestHttpAuthLogin).toHaveBeenCalled();
  });
});
