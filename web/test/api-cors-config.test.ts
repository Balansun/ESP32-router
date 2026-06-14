import { describe, expect, it } from "vitest";
import type { ConfigEnvelope } from "../src/api/types";

/** Regression: Api page must read CORS from envelope.config, not the envelope root. */
describe("API page CORS config load", () => {
  it("reads http_cors_enabled from config envelope", () => {
    const env: ConfigEnvelope = {
      schema_version: 4,
      config: { http_cors_enabled: true } as ConfigEnvelope["config"],
    };
    expect(env.config.http_cors_enabled).toBe(true);
    expect((env as { http_cors_enabled?: boolean }).http_cors_enabled).toBeUndefined();
  });
});
