import { describe, expect, it } from "vitest";
import type { RouterConfig } from "../src/api/types";
import { adaptBackupForTarget, isSparseBackup } from "../src/utils/adaptBackupForTarget";
import type { BalansunBackup } from "../src/utils/backupFormat";

const sparseCfg = { source: "Pmqtt", router_name: "Custom" } as RouterConfig;

const sparseBackup: BalansunBackup = {
  backupSchemaVersion: 1,
  exportedAt: "2026-01-01T00:00:00.000Z",
  device: { export_mode: "sparse" },
  config: sparseCfg,
  wifi: { ssid: "lab", password: "x" },
};

describe("adaptBackupForTarget", () => {
  it("detects sparse export mode", () => {
    expect(isSparseBackup(sparseBackup)).toBe(true);
  });

  it("does not expand sparse payload for constrained target", () => {
    const out = adaptBackupForTarget(sparseBackup, {
      tier: "constrained",
      psram: false,
      hist_max_points: 200,
      config_export_max_bytes: 20480,
      full_config_export: false,
    });
    expect(out.config?.dhcp_on).toBeUndefined();
    expect(out.config?.source).toBe("Pmqtt");
  });

  it("expands sparse payload for full tier target", () => {
    const out = adaptBackupForTarget(sparseBackup, {
      tier: "extended",
      psram: true,
      hist_max_points: 600,
      config_export_max_bytes: 49152,
      full_config_export: true,
    });
    expect(out.config?.dhcp_on).toBe(true);
    expect(out.config?.source).toBe("Pmqtt");
  });
});
