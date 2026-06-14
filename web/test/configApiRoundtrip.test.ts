import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import type { ConfigEnvelope, RouterConfig } from "../src/api/types";

/** In-memory config store exercised through real api.patchConfig / api.getConfig. */
describe("config API patch and get roundtrip", () => {
  let store: RouterConfig;

  beforeEach(() => {
    store = {
      router_name: "Balansun",
      pwm_mode: "off",
      pwm_gpio: -1,
    } as RouterConfig;

    vi.stubGlobal(
      "fetch",
      vi.fn(async (input: RequestInfo | URL, init?: RequestInit) => {
        const url = String(input);
        const method = (init?.method ?? "GET").toUpperCase();

        if (url.endsWith("/api/v1/config") && method === "GET") {
          const body: ConfigEnvelope = { schema_version: 5, config: { ...store } };
          return new Response(JSON.stringify(body), {
            status: 200,
            headers: { "Content-Type": "application/json" },
          });
        }

        if (url.endsWith("/api/v1/config") && method === "PATCH") {
          const partial = JSON.parse(String(init?.body ?? "{}")) as Partial<RouterConfig>;
          Object.assign(store, partial);
          return new Response(JSON.stringify({ ok: true }), {
            status: 200,
            headers: { "Content-Type": "application/json" },
          });
        }

        return new Response("not found", { status: 404 });
      }),
    );
  });

  afterEach(() => {
    vi.unstubAllGlobals();
    vi.resetModules();
  });

  it("patchConfig updates values returned by getConfig", async () => {
    const { api } = await import("../src/api/client");

    const before = await api.getConfig();
    expect(before.config.router_name).toBe("Balansun");
    expect(before.config.pwm_mode).toBe("off");

    await api.patchConfig({ router_name: "Bench-Router", pwm_mode: "independent", pwm_gpio: 4 });

    const after = await api.getConfig();
    expect(after.config.router_name).toBe("Bench-Router");
    expect(after.config.pwm_mode).toBe("independent");
    expect(after.config.pwm_gpio).toBe(4);

    await api.patchConfig({
      router_name: before.config.router_name ?? "Balansun",
      pwm_mode: before.config.pwm_mode ?? "off",
      pwm_gpio: before.config.pwm_gpio ?? -1,
    });

    const restored = await api.getConfig();
    expect(restored.config.router_name).toBe("Balansun");
    expect(restored.config.pwm_mode).toBe("off");
    expect(restored.config.pwm_gpio).toBe(-1);
  });
});
