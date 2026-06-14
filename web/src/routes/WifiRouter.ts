import type { RouteCtx } from "../router";
import { api } from "../api/client";
import { applyPublicBootstrap } from "../api/publicBootstrap";
import { go } from "../router";
import {
  needsHttpAuthLoginForPath,
  saveReturnTo,
  requestHttpAuthLogin,
} from "../auth/httpAuthGate";
import { isWifiApSetupFromPublic, WIFI_STATION_PATH } from "../wifi/wifiPaths";
import { mountWifiApSetup } from "./WifiApSetup";

/**
 * `/wifi` — captive portal entry. Redirects to `/wifi/station` when already on STA.
 */
export async function mountWifiRouter(ctx: RouteCtx) {
  try {
    const pub = await api.getPublic({ signal: ctx.signal, omitAuth: true });
    applyPublicBootstrap(pub);
    if (!isWifiApSetupFromPublic(pub.wifi)) {
      if (needsHttpAuthLoginForPath(WIFI_STATION_PATH)) {
        saveReturnTo(WIFI_STATION_PATH);
        await requestHttpAuthLogin();
        return;
      }
      await go(WIFI_STATION_PATH, { replace: true });
      return;
    }
  } catch {
    /* Unreachable public — stay on AP setup UI */
  }
  await mountWifiApSetup(ctx);
}
