import { describe, expect, it } from "vitest";
import { en } from "../src/i18n/locales/en";
import { fr } from "../src/i18n/locales/fr";

function leafPaths(obj: unknown, prefix = ""): string[] {
  if (obj === null || typeof obj !== "object" || Array.isArray(obj)) {
    return prefix ? [prefix] : [];
  }
  const out: string[] = [];
  for (const [key, value] of Object.entries(obj as Record<string, unknown>)) {
    const path = prefix ? `${prefix}.${key}` : key;
    if (value !== null && typeof value === "object" && !Array.isArray(value)) {
      out.push(...leafPaths(value, path));
    } else {
      out.push(path);
    }
  }
  return out.sort();
}

describe("locale parity", () => {
  it("fr has the same key paths as en", () => {
    const enPaths = leafPaths(en);
    const frPaths = leafPaths(fr);
    expect(frPaths).toEqual(enPaths);
  });
});
