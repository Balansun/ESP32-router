import { describe, expect, it } from "vitest";
import { readFileSync } from "node:fs";
import { resolve } from "node:path";

describe("Backup route i18n", () => {
  it("uses getStrings() instead of hardcoded French locale", () => {
    const src = readFileSync(resolve(__dirname, "../src/routes/Backup.ts"), "utf8");
    expect(src).toContain("getStrings()");
    expect(src).not.toMatch(/from\s+["']\.\.\/i18n\/fr["']/);
  });
});
