import type { RouteCtx } from "../router";
import { getStrings } from "../i18n";
import { mountWifiForm } from "../wifi/wifiForm";

/** Access-point / captive-portal Wi‑Fi setup — open API, no login. */
export async function mountWifiApSetup(ctx: RouteCtx) {
  const T = getStrings();
  await mountWifiForm({
    outlet: ctx.outlet,
    signal: ctx.signal,
    fetchOpts: { signal: ctx.signal, omitAuth: true },
    apMode: true,
    title: T.wifi.ap.title,
    description: T.wifi.ap.subtitle,
  });
}
