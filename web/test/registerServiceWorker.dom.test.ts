import { describe, expect, it, vi, afterEach } from "vitest";
import { registerServiceWorker } from "../src/pwa/registerServiceWorker";

describe("registerServiceWorker", () => {
  afterEach(() => {
    vi.unstubAllGlobals();
  });

  it("no-ops when serviceWorker is unavailable", async () => {
    vi.stubGlobal("navigator", {});
    await expect(registerServiceWorker()).resolves.toBeUndefined();
  });

  it("registers /sw.js when supported", async () => {
    const register = vi.fn().mockResolvedValue({
      addEventListener: vi.fn(),
      update: vi.fn().mockResolvedValue(undefined),
    });
    vi.stubGlobal("navigator", { serviceWorker: { register } });
    document.head.innerHTML = '<meta name="balansun-ui-version" content="1.2.3" />';
    await registerServiceWorker();
    expect(register).toHaveBeenCalledWith("/sw.js", { scope: "/" });
  });
});
