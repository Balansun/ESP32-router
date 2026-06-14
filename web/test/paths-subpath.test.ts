import { afterEach, describe, expect, it, vi } from "vitest";

describe("paths with subpath base", () => {
  afterEach(() => {
    vi.unstubAllEnvs();
    vi.resetModules();
  });

  it("stripBase and withBase honor non-root BASE", async () => {
    vi.stubEnv("BASE", "/app/");
    const { stripBase, withBase, toBrowserPath, normalizeBase } = await import(
      "../src/paths"
    );
    expect(normalizeBase()).toBe("/app/");
    expect(stripBase("/app/settings")).toBe("/settings");
    expect(withBase("/wifi")).toBe("/app/wifi");
    expect(toBrowserPath("/diag")).toBe("/app/diag");
    expect(stripBase("/app")).toBe("/");
    expect(stripBase("/app/")).toBe("/");
    expect(stripBase("/other")).toBe("/other");
    expect(toBrowserPath("/app/settings")).toBe("/app/settings");
    expect(withBase("settings")).toBe("/app/settings");
  });

  it("normalizeBase adds leading slash when missing", async () => {
    vi.stubEnv("BASE", "app/");
    const { normalizeBase } = await import("../src/paths");
    expect(normalizeBase()).toBe("/app/");
  });

  it("normalizeBase appends trailing slash when missing", async () => {
    vi.stubEnv("BASE", "/app");
    const { normalizeBase } = await import("../src/paths");
    expect(normalizeBase()).toBe("/app/");
  });

  it("stripBase returns rest without leading slash fix", async () => {
    vi.stubEnv("BASE", "/app/");
    const { stripBase } = await import("../src/paths");
    expect(stripBase("app/settings")).toBe("app/settings");
    expect(stripBase("/app/More/settings")).toBe("/More/settings");
  });

});
