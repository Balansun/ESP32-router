import { describe, expect, it } from "vitest";
import { validateRequiredGroups } from "../src/utils/pmqttMetricCatalog";

describe("pmqttMetricCatalog required groups", () => {
  it("accepts signed power group", () => {
    const v = validateRequiredGroups([
      { metric: "house.signed_net_w", topic: "x", format: "plain", enabled: true },
    ]);
    expect(v.ok).toBe(true);
  });

  it("accepts split import/export group", () => {
    const v = validateRequiredGroups([
      { metric: "house.active_import_w", topic: "x", format: "json", path: "import", enabled: true },
      { metric: "house.active_export_w", topic: "y", format: "json", path: "export", enabled: true },
    ]);
    expect(v.ok).toBe(true);
  });

  it("rejects missing required power group", () => {
    const v = validateRequiredGroups([
      { metric: "raw_meter.voltage_house_v", topic: "x", format: "json", path: "v", enabled: true },
    ]);
    expect(v.ok).toBe(false);
  });
});
