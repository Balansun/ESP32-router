import { describe, expect, it, beforeEach, vi } from "vitest";

vi.hoisted(() => {
  const storage = new Map<string, string>();
  vi.stubGlobal("localStorage", {
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

import { applyPublicBootstrap } from "../src/api/publicBootstrap";
import { publicBootstrap } from "../src/state/store";

describe("publicBootstrap", () => {
  beforeEach(() => {
    publicBootstrap.set({ ready: false, httpAuthEnabled: false });
  });

  it("applyPublicBootstrap sets ready and httpAuthEnabled from public info", () => {
    applyPublicBootstrap({
      http_auth: { enabled: true, username: "admin" },
      device: {
        router_name: "Test",
        firmware_version: "1.0",
        source_configured: true,
      },
      wifi: { mode: "sta", connected: true },
    });
    expect(publicBootstrap.get()).toEqual({
      ready: true,
      httpAuthEnabled: true,
      wifi: { mode: "sta", connected: true },
    });
  });
});
