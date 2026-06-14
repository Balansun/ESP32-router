import { describe, expect, it } from "vitest";
import { en } from "../src/i18n/locales/en";
import type { DeviceCapabilities } from "../src/api/types";
import {
  buildConstrainedTierBanner,
  formatConstrainedDiagBanner,
  isConstrainedDevice,
} from "../src/utils/deviceTier";

const constrainedCaps: DeviceCapabilities = {
  tier: "constrained",
  psram: false,
  hist_max_points: 200,
  history_days_retained: 7,
  history_power_window: "24h",
  config_export_max_bytes: 20480,
  full_config_export: false,
};

const standardCaps: DeviceCapabilities = {
  tier: "standard",
  psram: true,
  hist_max_points: 400,
  config_export_max_bytes: 32768,
  full_config_export: true,
};

describe("deviceTier constrained diag banner", () => {
  it("isConstrainedDevice only when tier is constrained", () => {
    expect(isConstrainedDevice(constrainedCaps)).toBe(true);
    expect(isConstrainedDevice(standardCaps)).toBe(false);
    expect(isConstrainedDevice(undefined)).toBe(false);
  });

  it("formatConstrainedDiagBanner substitutes capability values", () => {
    const text = formatConstrainedDiagBanner(en.diag.constrainedBanner, constrainedCaps);
    expect(text).toContain("7 days");
    expect(text).toContain("200 points");
    expect(text).toContain("24h");
    expect(text).not.toContain("{days}");
  });

  it("buildConstrainedTierBanner renders warn banner and backup link", () => {
    const el = buildConstrainedTierBanner(constrainedCaps);
    const banner = el.querySelector(".banner.banner--warn");
    expect(banner).not.toBeNull();
    expect(banner?.textContent).toContain("constrained mode");
    const link = el.querySelector("a[data-route]");
    expect(link?.getAttribute("href")).toContain("/settings/backup");
  });

  it("standard tier does not use constrained banner builder in Diag flow", () => {
    expect(isConstrainedDevice(standardCaps)).toBe(false);
  });
});
