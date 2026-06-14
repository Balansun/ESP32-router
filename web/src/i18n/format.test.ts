import { describe, expect, it } from "vitest";
import { formatStr } from "./format";

describe("formatStr", () => {
  it("replaces placeholders", () => {
    expect(formatStr("Hello {name}", { name: "World" })).toBe("Hello World");
    expect(formatStr("{ms} ms ago", { ms: 42 })).toBe("42 ms ago");
  });

  it("leaves unknown placeholders unchanged", () => {
    expect(formatStr("Missing {x}", {})).toBe("Missing {x}");
  });
});
