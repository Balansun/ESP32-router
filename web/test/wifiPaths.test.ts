import { describe, expect, it, beforeEach } from "vitest";
import {
  isWifiApSetupFromPublic,
  wifiNavPath,
  WIFI_AP_PATH,
  WIFI_STATION_PATH,
} from "../src/wifi/wifiPaths";
import { publicBootstrap } from "../src/state/store";

describe("wifiPaths", () => {
  beforeEach(() => {
    publicBootstrap.set({ ready: false, httpAuthEnabled: false });
  });

  it("defaults nav to station path when bootstrap not ready", () => {
    expect(wifiNavPath()).toBe(WIFI_STATION_PATH);
  });

  it("nav to AP path when public says AP or disconnected STA", () => {
    publicBootstrap.set({
      ready: true,
      httpAuthEnabled: false,
      wifi: { mode: "ap", connected: false },
    });
    expect(wifiNavPath()).toBe(WIFI_AP_PATH);

    publicBootstrap.set({
      ready: true,
      httpAuthEnabled: false,
      wifi: { mode: "sta", connected: false },
    });
    expect(wifiNavPath()).toBe(WIFI_AP_PATH);
  });

  it("nav to station path when STA connected", () => {
    publicBootstrap.set({
      ready: true,
      httpAuthEnabled: true,
      wifi: { mode: "sta", connected: true },
    });
    expect(wifiNavPath()).toBe(WIFI_STATION_PATH);
  });

  it("isWifiApSetupFromPublic", () => {
    expect(isWifiApSetupFromPublic(undefined)).toBe(true);
    expect(isWifiApSetupFromPublic({ mode: "ap", connected: false })).toBe(true);
    expect(isWifiApSetupFromPublic({ mode: "sta", connected: false })).toBe(true);
    expect(isWifiApSetupFromPublic({ mode: "sta", connected: true })).toBe(false);
    expect(
      isWifiApSetupFromPublic({ mode: "sta", connected: true, setup_ap: true }),
    ).toBe(true);
  });
});
