import { describe, expect, it, vi } from "vitest";
import {
  applyBrandAssets,
  brandFaviconDataUrl,
  brandThemeFromPref,
  installBrandAssets,
} from "../src/brand/brandAssets";

describe("brandAssets", () => {
  it("brandThemeFromPref resolves light and dark", () => {
    expect(brandThemeFromPref("light")).toBe("light");
    expect(brandThemeFromPref("dark")).toBe("dark");
  });

  it("brandThemeFromPref follows system when auto (dark)", () => {
    vi.stubGlobal("matchMedia", () => ({
      matches: true,
      addEventListener: () => {},
      removeEventListener: () => {},
    }));
    expect(brandThemeFromPref("auto")).toBe("dark");
    vi.unstubAllGlobals();
  });

  it("brandThemeFromPref follows system when auto (light)", () => {
    vi.stubGlobal("matchMedia", () => ({
      matches: false,
      addEventListener: () => {},
      removeEventListener: () => {},
    }));
    expect(brandThemeFromPref("auto")).toBe("light");
    vi.unstubAllGlobals();
  });

  it("installBrandAssets sets favicon link", () => {
    installBrandAssets("light");
    const icon = document.querySelector('link[rel="icon"]');
    expect(icon).not.toBeNull();
    expect(brandFaviconDataUrl("light")).toContain("data:image/svg+xml");
  });

  it("applyBrandAssets returns effective theme", () => {
    const theme = applyBrandAssets("light");
    expect(theme).toBe("light");
  });
});
