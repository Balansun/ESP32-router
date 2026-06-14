import type { RouteCtx } from "../router";
import { getStrings } from "../i18n";
import { mountWifiForm } from "../wifi/wifiForm";

/** Wi‑Fi management while connected on your LAN — uses HTTP API session when enabled. */
export async function mountWifiStation(ctx: RouteCtx) {
  const T = getStrings();
  await mountWifiForm({
    outlet: ctx.outlet,
    signal: ctx.signal,
    fetchOpts: { signal: ctx.signal, omitAuth: false },
    apMode: false,
    title: T.wifi.station.title,
    description: T.wifi.station.subtitle,
  });
}
