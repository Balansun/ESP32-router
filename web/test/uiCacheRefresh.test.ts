// @vitest-environment happy-dom
import { describe, expect, it, vi, beforeEach } from "vitest";
import {
  readEmbeddedUiVersion,
  shouldSkipUiCacheCheck,
  decideUiCacheRefresh,
} from "../src/pwa/uiCacheRefresh";

describe("uiCacheRefresh", () => {
  beforeEach(() => {
    document.head.replaceChildren();
  });

  it("shouldSkipUiCacheCheck skips dev and empty", () => {
    expect(shouldSkipUiCacheCheck("dev")).toBe(true);
    expect(shouldSkipUiCacheCheck("")).toBe(true);
    expect(shouldSkipUiCacheCheck("%VITE_FIRMWARE_VERSION%")).toBe(true);
    expect(shouldSkipUiCacheCheck("0.3.6")).toBe(false);
  });

  it("readEmbeddedUiVersion prefers meta tag", () => {
    const meta = document.createElement("meta");
    meta.name = "balansun-ui-version";
    meta.content = "0.3.5";
    document.head.appendChild(meta);
    expect(readEmbeddedUiVersion(document)).toBe("0.3.5");
  });

  it("decideUiCacheRefresh continues when versions match", () => {
    expect(decideUiCacheRefresh("0.3.6", "0.3.6", false)).toEqual({ action: "continue" });
    expect(decideUiCacheRefresh("v0.3.6", "0.3.6_RMS", false)).toEqual({ action: "continue" });
  });

  it("decideUiCacheRefresh reloads once on mismatch", () => {
    vi.stubGlobal("location", { pathname: "/settings" });
    const first = decideUiCacheRefresh("0.3.5", "0.3.6", false);
    expect(first.action).toBe("reload");
    expect(first.reloadUrl).toBe("/settings?ui=0.3.6");

    const second = decideUiCacheRefresh("0.3.5", "0.3.6", true);
    expect(second).toEqual({ action: "continue" });
    vi.unstubAllGlobals();
  });

  it("decideUiCacheRefresh continues when API version missing", () => {
    expect(decideUiCacheRefresh("0.3.6", null, false)).toEqual({ action: "continue" });
  });
});
