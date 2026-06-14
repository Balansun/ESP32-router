import type { PublicInfo } from "./types";
import { publicBootstrap } from "../state/store";

export function applyPublicBootstrap(pub: PublicInfo): void {
  publicBootstrap.set({
    ready: true,
    httpAuthEnabled: pub.http_auth.enabled,
    wifi: pub.wifi,
  });
}
