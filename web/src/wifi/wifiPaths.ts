import { publicBootstrap } from "../state/store";

/** Captive portal / first-run setup (open API, no login gate). */
export const WIFI_AP_PATH = "/wifi";

/** Change Wi‑Fi while device is on your LAN (STA); uses HTTP API session when enabled. */
export const WIFI_STATION_PATH = "/wifi/station";

export function isWifiApSetupFromPublic(
  wifi: { mode: "ap" | "sta"; connected: boolean; setup_ap?: boolean } | undefined,
): boolean {
  if (!wifi) return true;
  if (wifi.setup_ap === true) return true;
  return wifi.mode !== "sta" || !wifi.connected;
}

/** Nav link target: AP setup vs station management. */
export function wifiNavPath(): string {
  const boot = publicBootstrap.get();
  if (!boot.ready) return WIFI_STATION_PATH;
  if (isWifiApSetupFromPublic(boot.wifi)) return WIFI_AP_PATH;
  return WIFI_STATION_PATH;
}
