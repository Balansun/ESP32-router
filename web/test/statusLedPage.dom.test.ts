import { describe, expect, it } from "vitest";
import { getStrings } from "../src/i18n";
import { buildStatusLedPage } from "../src/routes/settings/statusLedSettings";
import type { RouterConfig } from "../src/api/types";

const baseCfg = {
  status_led_mode: "dual_gpio",
  status_led_gpio_activity: 18,
  status_led_gpio_regulation: 19,
} as RouterConfig;

describe("buildStatusLedPage", () => {
  it("shows four color pickers on S3 even with legacy dual_gpio config", () => {
    const ac = new AbortController();
    const page = buildStatusLedPage(baseCfg, getStrings(), ac.signal, { esp32s3: true });
    const grid = page.pageRoot.querySelector(".status-led-color-grid");
    expect(grid).not.toBeNull();
    expect(grid!.querySelectorAll('input[type="color"]')).toHaveLength(4);
    expect(page.pageRoot.querySelector(".status-led-chip-badge")).not.toBeNull();
    ac.abort();
  });

  it("shows dual-GPIO hint when mode is dual_gpio on WROOM", () => {
    const ac = new AbortController();
    const page = buildStatusLedPage(baseCfg, getStrings(), ac.signal, { esp32s3: false });
    const hint = page.pageRoot.querySelector(".status-led-colors-dual-hint") as HTMLElement;
    expect(hint).not.toBeNull();
    expect(hint.hidden).toBe(false);
    ac.abort();
  });
});
