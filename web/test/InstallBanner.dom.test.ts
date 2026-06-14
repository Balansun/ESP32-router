import { describe, expect, it, vi, afterEach } from "vitest";
import {
  buildInstallBanner,
  dismissInstallBanner,
  isInstallBannerDismissed,
  isMobileInstallTarget,
  isStandaloneDisplayMode,
  mountInstallBanner,
  shouldShowInstallBanner,
} from "../src/components/InstallBanner";
import { localePref } from "../src/state/store";

describe("InstallBanner", () => {
  afterEach(() => {
    vi.unstubAllGlobals();
    try {
      localStorage.removeItem("balansun_install_banner_dismissed");
    } catch {
      /* ignore */
    }
  });

  it("shouldShowInstallBanner hides in standalone mode", () => {
    expect(
      shouldShowInstallBanner({ standalone: true, mobile: true, dismissed: false }),
    ).toBe(false);
  });

  it("shouldShowInstallBanner hides when dismissed", () => {
    expect(
      shouldShowInstallBanner({ standalone: false, mobile: true, dismissed: true }),
    ).toBe(false);
  });

  it("shouldShowInstallBanner shows on mobile when not dismissed", () => {
    expect(
      shouldShowInstallBanner({ standalone: false, mobile: true, dismissed: false }),
    ).toBe(true);
  });

  it("dismissInstallBanner persists flag", () => {
    const storage = {
      data: {} as Record<string, string>,
      getItem(k: string) {
        return this.data[k] ?? null;
      },
      setItem(k: string, v: string) {
        this.data[k] = v;
      },
    } as Storage;
    dismissInstallBanner(storage);
    expect(isInstallBannerDismissed(storage)).toBe(true);
  });

  it("buildInstallBanner renders dismiss control", () => {
    vi.stubGlobal("navigator", { userAgent: "iPhone" });
    const { banner } = buildInstallBanner(() => {});
    expect(banner.className).toBe("install-banner");
    expect(banner.querySelector(".install-banner__dismiss")).not.toBeNull();
    expect(banner.textContent).toContain("Share");
  });

  it("mountInstallBanner prepends to container when eligible", () => {
    vi.stubGlobal("navigator", { userAgent: "Android", standalone: false });
    vi.stubGlobal(
      "matchMedia",
      vi.fn((query: string) => ({
        matches: query.includes("max-width") || query.includes("pointer: coarse"),
        addEventListener: () => {},
        removeEventListener: () => {},
      })),
    );
    const container = document.createElement("div");
    const cleanup = mountInstallBanner(container);
    expect(container.querySelector(".install-banner")).not.toBeNull();
    cleanup();
    expect(container.querySelector(".install-banner")).toBeNull();
  });

  it("isStandaloneDisplayMode returns false when window is undefined", () => {
    const win = globalThis.window;
    // @ts-expect-error SSR guard
    delete (globalThis as { window?: Window }).window;
    try {
      expect(isStandaloneDisplayMode()).toBe(false);
    } finally {
      globalThis.window = win;
    }
  });

  it("isMobileInstallTarget returns false when window is undefined", () => {
    const win = globalThis.window;
    // @ts-expect-error SSR guard
    delete (globalThis as { window?: Window }).window;
    try {
      expect(isMobileInstallTarget()).toBe(false);
    } finally {
      globalThis.window = win;
    }
  });

  it("isMobileInstallTarget returns false on desktop without coarse+narrow", () => {
    vi.stubGlobal("navigator", { userAgent: "Mozilla/5.0 (Windows NT 10.0)" });
    vi.stubGlobal(
      "matchMedia",
      vi.fn(() => ({ matches: false, addEventListener: () => {}, removeEventListener: () => {} })),
    );
    expect(isMobileInstallTarget()).toBe(false);
  });

  it("buildInstallBanner uses generic hint when userAgent is empty", () => {
    vi.stubGlobal("navigator", { userAgent: "" });
    const { banner } = buildInstallBanner(() => {});
    expect(banner.textContent).toContain("home screen");
  });

  it("isMobileInstallTarget treats missing userAgent as empty", () => {
    vi.stubGlobal("navigator", {});
    vi.stubGlobal(
      "matchMedia",
      vi.fn(() => ({ matches: false, addEventListener: () => {}, removeEventListener: () => {} })),
    );
    expect(isMobileInstallTarget()).toBe(false);
  });

  it("shouldShowInstallBanner evaluates live environment when opts omitted", () => {
    vi.stubGlobal("navigator", { userAgent: "Android", standalone: false });
    vi.stubGlobal(
      "matchMedia",
      vi.fn((query: string) => ({
        matches: query.includes("max-width") || query.includes("pointer: coarse"),
        addEventListener: () => {},
        removeEventListener: () => {},
      })),
    );
    expect(shouldShowInstallBanner()).toBe(true);
  });

  it("isStandaloneDisplayMode detects standalone or fullscreen media query", () => {
    vi.stubGlobal("navigator", { userAgent: "Desktop", standalone: false });
    vi.stubGlobal(
      "matchMedia",
      vi.fn((q: string) => ({
        matches: q.includes("standalone") || q.includes("fullscreen"),
        addEventListener: () => {},
        removeEventListener: () => {},
      })),
    );
    expect(isStandaloneDisplayMode()).toBe(true);
  });

  it("isMobileInstallTarget detects mobile UA", () => {
    vi.stubGlobal("navigator", { userAgent: "Mozilla/5.0 (iPhone)" });
    vi.stubGlobal(
      "matchMedia",
      vi.fn(() => ({ matches: false, addEventListener: () => {}, removeEventListener: () => {} })),
    );
    expect(isMobileInstallTarget()).toBe(true);
  });

  it("isStandaloneDisplayMode detects iOS standalone flag", () => {
    vi.stubGlobal("navigator", { userAgent: "iPhone", standalone: true });
    vi.stubGlobal(
      "matchMedia",
      vi.fn(() => ({ matches: false, addEventListener: () => {}, removeEventListener: () => {} })),
    );
    expect(isStandaloneDisplayMode()).toBe(true);
  });

  it("isMobileInstallTarget detects coarse pointer and narrow viewport", () => {
    vi.stubGlobal("navigator", { userAgent: "Desktop" });
    vi.stubGlobal(
      "matchMedia",
      vi.fn((q: string) => ({
        matches: q.includes("pointer: coarse") || q.includes("max-width"),
        addEventListener: () => {},
        removeEventListener: () => {},
      })),
    );
    expect(isMobileInstallTarget()).toBe(true);
  });

  it("isInstallBannerDismissed returns false when storage throws", () => {
    const storage = {
      getItem: () => {
        throw new Error("blocked");
      },
      setItem: () => {},
    } as Storage;
    expect(isInstallBannerDismissed(storage)).toBe(false);
  });

  it("dismissInstallBanner ignores storage write failures", () => {
    const storage = {
      getItem: () => null,
      setItem: () => {
        throw new Error("blocked");
      },
    } as Storage;
    expect(() => dismissInstallBanner(storage)).not.toThrow();
  });

  it("buildInstallBanner uses Android and generic hints", () => {
    vi.stubGlobal("navigator", { userAgent: "Mozilla/5.0 (Linux; Android 14)" });
    const { banner: android } = buildInstallBanner(() => {});
    expect(android.textContent).toContain("home screen icon");

    vi.stubGlobal("navigator", { userAgent: "Mozilla/5.0 (Windows NT 10.0)" });
    const { banner: generic } = buildInstallBanner(() => {});
    expect(generic.textContent).toContain("Install");
  });

  it("buildInstallBanner refreshes copy when locale changes", () => {
    vi.stubGlobal("navigator", { userAgent: "iPhone" });
    localePref.set("en");
    const { banner } = buildInstallBanner(() => {});
    expect(banner.textContent).toContain("Add to Home Screen");

    localePref.set("fr");
    expect(banner.textContent).toContain("Installez Balansun");
    localePref.set("en");
  });

  it("mountInstallBanner no-ops when not eligible", () => {
    vi.stubGlobal("navigator", { userAgent: "Windows NT", standalone: false });
    vi.stubGlobal(
      "matchMedia",
      vi.fn(() => ({ matches: false, addEventListener: () => {}, removeEventListener: () => {} })),
    );
    const container = document.createElement("div");
    const cleanup = mountInstallBanner(container);
    expect(container.querySelector(".install-banner")).toBeNull();
    expect(cleanup()).toBeUndefined();
  });

  it("mountInstallBanner dismiss removes banner and persists flag", () => {
    vi.stubGlobal("navigator", { userAgent: "Android", standalone: false });
    vi.stubGlobal(
      "matchMedia",
      vi.fn((query: string) => ({
        matches: query.includes("max-width") || query.includes("pointer: coarse"),
        addEventListener: () => {},
        removeEventListener: () => {},
      })),
    );
    const storage = {
      data: {} as Record<string, string>,
      getItem(k: string) {
        return this.data[k] ?? null;
      },
      setItem(k: string, v: string) {
        this.data[k] = v;
      },
    } as Storage;
    vi.stubGlobal("localStorage", storage);
    const container = document.createElement("div");
    mountInstallBanner(container);
    const btn = container.querySelector<HTMLButtonElement>(".install-banner__dismiss");
    expect(btn).not.toBeNull();
    btn!.click();
    expect(container.querySelector(".install-banner")).toBeNull();
    expect(isInstallBannerDismissed(storage)).toBe(true);
  });
});
