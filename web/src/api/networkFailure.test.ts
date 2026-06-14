import { describe, expect, it } from "vitest";
import { ApiNetworkError, isBrowserNetworkFailure } from "./networkFailure";

describe("isBrowserNetworkFailure", () => {
  it("detects TypeError Failed to fetch", () => {
    expect(isBrowserNetworkFailure(new TypeError("Failed to fetch"))).toBe(true);
  });

  it("detects ApiNetworkError", () => {
    expect(isBrowserNetworkFailure(new ApiNetworkError("device"))).toBe(true);
  });
});
