import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";

const __dirname = dirname(fileURLToPath(import.meta.url));

const EXPECTED_PATHS = [
  "/api/v1/auth/login",
  "/api/v1/auth/logout",
  "/api/v1/auth/tokens",
  "/api/v1/auth/tokens/{id}",
  "/api/v1/actions",
  "/api/v1/actions/config",
  "/api/v1/actions/config/{index}",
  "/api/v1/actions/schema",
  "/api/v1/actions/{index}/override",
  "/api/v1/actions/{index}/override/clear",
  "/api/v1/config",
  "/api/v1/device",
  "/api/v1/firmware/ota",
  "/api/v1/fleet/export",
  "/api/v1/fleet/import",
  "/api/v1/fleet/trust-key",
  "/api/v1/gpio",
  "/api/v1/health",
  "/api/v1/health/hardware/recheck",
  "/api/v1/hardware/status-led/test",
  "/api/v1/health/self-test/run",
  "/api/v1/health/self-test/simulate",
  "/api/v1/health/self-test/skip",
  "/api/v1/history/energy/daily",
  "/api/v1/history/energy/daily/import",
  "/api/v1/history/power",
  "/api/v1/history/reset",
  "/api/v1/measurements",
  "/api/v1/mqtt/discover",
  "/api/v1/mqtt/publish-now",
  "/api/v1/mqtt/reconnect",
  "/api/v1/mqtt/test",
  "/api/v1/openapi.json",
  "/api/v1/public",
  "/api/v1/pwm",
  "/api/v1/sources",
  "/api/v1/sources/brute_panel",
  "/api/v1/sources/diagnostics",
  "/api/v1/sources/pmqtt/preview",
  "/api/v1/sources/test/inject",
  "/api/v1/state",
  "/api/v1/system",
  "/api/v1/system/arduino-ota",
  "/api/v1/system/audit",
  "/api/v1/system/backup",
  "/api/v1/system/backup/manifest",
  "/api/v1/system/backup/part/{part}",
  "/api/v1/system/eeprom",
  "/api/v1/system/factory-reset",
  "/api/v1/system/http-auth",
  "/api/v1/system/pins",
  "/api/v1/system/pins/reset",
  "/api/v1/system/reboot",
  "/api/v1/system/save-now",
  "/api/v1/tariff/tempo",
  "/api/v1/telemetry/snapshot",
  "/api/v1/time",
  "/api/v1/triac/override",
  "/api/v1/wifi",
  "/api/v1/wifi/scan",
].sort();

describe("OpenAPI contract snapshot", () => {
  it("matches canonical balansun-v1.yaml (regenerate via scripts/sync_openapi_snapshot.py)", () => {
    const raw = readFileSync(join(__dirname, "fixtures/openapi-snapshot.json"), "utf8");
    const doc = JSON.parse(raw) as {
      openapi: string;
      info: { title: string; version: string };
      paths: Record<string, unknown>;
      components?: { schemas?: Record<string, unknown> };
    };
    expect(doc.openapi).toBe("3.0.0");
    expect(doc.info.title).toBe("Balansun API");
    expect(Object.keys(doc.paths).sort()).toEqual(EXPECTED_PATHS);
    expect(doc.components?.schemas?.Measurements).toBeDefined();
    expect(doc.paths["/api/v1/measurements"]).toMatchObject({
      get: expect.objectContaining({ operationId: "getMeasurements" }),
    });
  });
});
