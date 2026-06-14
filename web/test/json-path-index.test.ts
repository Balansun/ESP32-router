import { describe, expect, it } from "vitest";
import { getByJsonPath, indexJsonPaths, normalizeJsonPath } from "../src/utils/jsonPathIndex";

describe("jsonPathIndex", () => {
  it("normalizes jsonpath syntax", () => {
    expect(normalizeJsonPath("$.house.Pw")).toBe("house.Pw");
    expect(normalizeJsonPath("house.Pw")).toBe("house.Pw");
    expect(normalizeJsonPath("$")).toBe("");
  });

  it("indexes nested paths", () => {
    const entries = indexJsonPaths({ house: { Pw: -1200, Pf: 0.98 } });
    expect(entries.some((e) => e.pathDisplay === "$.house.Pw")).toBe(true);
    expect(entries.some((e) => e.pathDisplay === "$.house.Pf")).toBe(true);
  });

  it("reads values by path", () => {
    expect(getByJsonPath({ house: { Pw: -1200 } }, "$.house.Pw")).toBe(-1200);
  });

  it("indexes arrays, null, and booleans", () => {
    const entries = indexJsonPaths({ items: [1, null], ok: true, name: "x" });
    expect(entries.some((e) => e.kind === "array")).toBe(true);
    expect(entries.some((e) => e.kind === "null")).toBe(true);
    expect(entries.some((e) => e.kind === "boolean")).toBe(true);
  });

  it("getByJsonPath reads array index and empty path", () => {
    const root = { list: [10, 20] };
    expect(getByJsonPath(root, "")).toEqual(root);
    expect(getByJsonPath(root, "$.list.1")).toBe(20);
    expect(getByJsonPath(null, "$.a")).toBeUndefined();
  });
});
