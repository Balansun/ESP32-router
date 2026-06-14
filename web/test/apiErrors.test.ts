import { describe, expect, it, vi, beforeEach } from "vitest";
import { ApiError } from "../src/api/client";
import { formatApiError, mapFirmwareApiMessage } from "../src/api/apiErrors";
import { localePref } from "../src/state/store";

describe("apiErrors", () => {
  beforeEach(() => {
    localePref.set("en");
  });

  it("mapFirmwareApiMessage maps HTTP API password not enabled", () => {
    const msg = mapFirmwareApiMessage("HTTP API password is not enabled");
    expect(msg).toContain("HTTP API password");
    expect(msg).not.toMatch(/^HTTP 400/);
  });

  it("mapFirmwareApiMessage maps max tokens", () => {
    expect(mapFirmwareApiMessage("maximum number of tokens reached")).toContain("4 tokens");
  });

  it("formatApiError uses mapped message from ApiError body", () => {
    const e = new ApiError("HTTP 400 Bad Request", 400, {
      error: "bad_request",
      message: "HTTP API password is not enabled",
    });
    const text = formatApiError(e);
    expect(text).not.toBe("HTTP 400 Bad Request");
    expect(text).toContain("HTTP API password");
  });

  it("formatApiError falls back to body.message when unknown", () => {
    const e = new ApiError("HTTP 400 Bad Request", 400, {
      message: "custom device message",
    });
    expect(formatApiError(e)).toBe("custom device message");
  });

  it("formatApiError uses French mapping when locale is fr", () => {
    localePref.set("fr");
    const text = formatApiError(
      new ApiError("HTTP 400", 400, {
        message: "HTTP API password is not enabled",
      }),
    );
    expect(text).toContain("mot de passe API");
  });
});
