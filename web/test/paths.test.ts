import { describe, expect, it } from "vitest";
import { normalizeBase, stripBase, toBrowserPath, withBase } from "../src/paths";

describe("paths", () => {
  it("normalizeBase defaults to slash", () => {
    expect(normalizeBase()).toBe("/");
  });

  it("withBase preserves logical routes at root base", () => {
    expect(withBase("/settings")).toBe("/settings");
    expect(withBase("settings")).toBe("/settings");
  });

  it("stripBase returns pathname at root base", () => {
    expect(stripBase("/settings")).toBe("/settings");
    expect(stripBase("")).toBe("/");
  });

  it("toBrowserPath returns path under root base", () => {
    expect(toBrowserPath("/wifi")).toBe("/wifi");
    expect(toBrowserPath("wifi")).toBe("/wifi");
  });
});
