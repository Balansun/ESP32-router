import { hasSession, logoutSession } from "../api/apiSession";
import { normalizeLogicalPath, stripBase } from "../paths";
import { WIFI_AP_PATH } from "../wifi/wifiPaths";
import { publicBootstrap } from "../state/store";

const RETURN_TO_KEY = "balansun_return_to";

export const LOGIN_PATH = "/login";

/** Only AP captive-portal setup — not `/wifi/station`. */
export function isAuthExemptPath(pathname: string): boolean {
  return normalizeLogicalPath(pathname) === WIFI_AP_PATH;
}

export function isLoginPath(pathname: string): boolean {
  return stripBase(pathname || "/") === LOGIN_PATH;
}

/** True when API password is on and the browser has no session token (any path). */
export function requiresHttpAuthSession(): boolean {
  const boot = publicBootstrap.get();
  return boot.ready && boot.httpAuthEnabled && !hasSession();
}

export function needsHttpAuthLoginForPath(pathname: string): boolean {
  if (isAuthExemptPath(pathname) || isLoginPath(pathname)) return false;
  return requiresHttpAuthSession();
}

export function needsHttpAuthLogin(): boolean {
  if (typeof location === "undefined") return false;
  return needsHttpAuthLoginForPath(location.pathname || "/");
}

export function saveReturnTo(path: string): void {
  const logical = path.startsWith("/") ? path : `/${path}`;
  const stripped = stripBase(logical);
  if (isLoginPath(stripped)) return;
  try {
    sessionStorage.setItem(RETURN_TO_KEY, stripped);
  } catch {
    // ignore quota / private mode
  }
}

export function consumeReturnTo(): string {
  try {
    const p = sessionStorage.getItem(RETURN_TO_KEY);
    sessionStorage.removeItem(RETURN_TO_KEY);
    if (p && p.startsWith("/") && !isLoginPath(p)) return p;
  } catch {
    // ignore
  }
  return "/";
}

let authRequiredHandler: (() => Promise<boolean>) | null = null;

export function registerAuthRequiredHandler(handler: () => Promise<boolean>): void {
  authRequiredHandler = handler;
}

/** Clear session and show the login page; resolves true when signed in again. */
export async function requestHttpAuthLogin(): Promise<boolean> {
  await logoutSession();
  if (authRequiredHandler) return authRequiredHandler();
  return false;
}
