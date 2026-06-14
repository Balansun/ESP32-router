import { applyBrandAssets } from "./brand/brandAssets";
import "./styles/tokens.css";
import "./styles/base.css";
import "uplot/dist/uPlot.min.css";
import "./styles/layout.css";
import "./styles/components.css";
import "./styles/routes.css";

import { startConnHealthMonitor } from "./state/connStatus";
import { hydrateDeviceInfoFromApi } from "./state/deviceInfoHydrate";
import { productModePioEnv } from "./state/productMode";
import { showActionsNav } from "./utils/firmwareCaps";
import { applyTheme, deviceInfo, localePref, themePref } from "./state/store";
import { installPwaManifest } from "./pwa/installManifest";
import { registerServiceWorker } from "./pwa/registerServiceWorker";
import { configure, currentPath, go, register, rerender, setRouteAuthGuard, start } from "./router";
import { isSettingsPath } from "./routes/settings/settingsPaths";
import { buildShell } from "./components/Shell";
import { mountHome } from "./routes/Home";
import { mountHistory } from "./routes/History";
import { mountActions } from "./routes/Actions";
import { mountDiag } from "./routes/Diag";
import { mountWifiRouter } from "./routes/WifiRouter";
import { mountWifiStation } from "./routes/WifiStation";
import { mountFirmware } from "./routes/Firmware";
import { mountApi } from "./routes/Api";
import { api } from "./api/client";
import { applyPublicBootstrap } from "./api/publicBootstrap";
import { clearSession, probeApiSession } from "./api/apiSession";
import {
  consumeReturnTo,
  isAuthExemptPath,
  isLoginPath,
  LOGIN_PATH,
  needsHttpAuthLoginForPath,
  registerAuthRequiredHandler,
  requiresHttpAuthSession,
  saveReturnTo,
} from "./auth/httpAuthGate";
import { publicBootstrap } from "./state/store";
import { mountLoginPage } from "./routes/Login";
import { stripBase, toBrowserPath } from "./paths";
import { WIFI_AP_PATH } from "./wifi/wifiPaths";
import { toast } from "./components/Toast";
import { isBrowserNetworkFailure } from "./api/networkFailure";
import { getStrings } from "./i18n";
import {
  hydrateFirmwareUpdateFromStorage,
  runFirmwareUpdateDailyCheck,
} from "./firmware/githubDailyCheck";

applyTheme(themePref.get());
applyBrandAssets(themePref.get());
themePref.subscribe((p) => {
  applyBrandAssets(p);
  if (publicBootstrap.get().ready) {
    installPwaManifest(deviceInfo.get()?.router_name);
  }
});
if (typeof window !== "undefined") {
  window.matchMedia("(prefers-color-scheme: dark)").addEventListener("change", () => {
    if (themePref.get() === "auto") applyBrandAssets("auto");
  });
}

const BOOTSTRAP_RETRY_MS = 2500;

async function fetchPublicBootstrap(): Promise<void> {
  const pub = await api.getPublic({ timeoutMs: 6000 });
  applyPublicBootstrap(pub);
  deviceInfo.set({
    router_name: pub.device.router_name,
    firmware_version: pub.device.firmware_version,
    probe_house_name: deviceInfo.get()?.probe_house_name ?? "",
    probe_second_name: deviceInfo.get()?.probe_second_name ?? "",
    temperature_label: deviceInfo.get()?.temperature_label ?? "",
  });
  await registerServiceWorker();
  installPwaManifest(pub.device.router_name);
}

/** Load public bootstrap before protected API calls. Skip on `/wifi` (AP setup only). */
async function bootstrapUi(): Promise<void> {
  if (isAuthExemptPath(location.pathname || "/")) return;
  try {
    await fetchPublicBootstrap();
  } catch {
    await new Promise((r) => setTimeout(r, BOOTSTRAP_RETRY_MS));
    try {
      await fetchPublicBootstrap();
    } catch {
      // Unreachable device — shell may start; router guard blocks once auth is known.
    }
  }
}

const appRoot = document.getElementById("app")!;

function markAppReady(): void {
  appRoot.removeAttribute("aria-busy");
}

function syncLoginUrl(): void {
  if (stripBase(location.pathname || "/") === LOGIN_PATH) return;
  history.replaceState({}, "", toBrowserPath(LOGIN_PATH));
}

let routesRegistered = false;
let globalHandlersInstalled = false;
let localeSubscribed = false;
let stopConnHealth: () => void = () => {};
let loginPromise: Promise<boolean> | null = null;

function teardownMainApp(): void {
  stopConnHealth();
  stopConnHealth = () => {};
  document.querySelector(".layout")?.remove();
  document.querySelector("nav.tabbar")?.remove();
  appRoot.replaceChildren();
}

function registerRoutes(): void {
  if (routesRegistered) return;
  routesRegistered = true;
  register("/", mountHome);
  register("/history", mountHistory);
  register("/actions", mountActions);
  // Lazy imports do not reduce single-file bundle size (inlineDynamicImports); they keep initial parse smaller in dev only.
  register("/settings", async (ctx) => (await import("./routes/Settings")).mountSettings(ctx), {
    layout: "settings",
  });
  register("/api", mountApi);
  register("/backup", async (ctx) => (await import("./routes/Backup")).mountBackup(ctx));
  register("/diag", mountDiag);
  register("/wifi", mountWifiRouter);
  register("/wifi/station", mountWifiStation);
  register("/firmware", mountFirmware);
  register("*", mountHome);
}

function globalErrorToastMessage(reason: unknown): string | null {
  if (isBrowserNetworkFailure(reason)) {
    return getStrings().status.error;
  }
  const detail =
    reason instanceof Error
      ? reason.message
      : typeof reason === "string"
        ? reason
        : "";
  const trimmed = detail.trim();
  if (!trimmed) return null;
  if (/failed to fetch|networkerror|load failed|fetch failed/i.test(trimmed)) {
    return getStrings().status.error;
  }
  return trimmed;
}

function installGlobalErrorHandlers(): void {
  if (globalHandlersInstalled) return;
  globalHandlersInstalled = true;
  const show = (msg: string) => toast(msg, "error");
  window.addEventListener("error", (ev) => {
    const msg = globalErrorToastMessage(ev.error ?? ev.message);
    if (msg) show(msg);
  });
  window.addEventListener("unhandledrejection", (ev) => {
    const msg = globalErrorToastMessage(ev.reason);
    if (!msg) return;
    if (isBrowserNetworkFailure(ev.reason)) ev.preventDefault();
    show(msg);
  });
}

function installRouteAuthGuard(): void {
  setRouteAuthGuard((targetPath) => {
    if (isLoginPath(targetPath)) return true;
    if (!needsHttpAuthLoginForPath(targetPath)) return true;
    void redirectToLogin(targetPath);
    return false;
  });
}

async function refreshDeviceInfo(): Promise<void> {
  try {
    const d = await api.getDevice({ retry: 2 });
    hydrateDeviceInfoFromApi(d);
  } catch {
    // Non-fatal — UI still renders with defaults.
  }
}

function startMainApp(): void {
  loginCleanup?.();
  loginCleanup = null;
  teardownMainApp();
  const apSetupShell =
    typeof location !== "undefined" &&
    stripBase(location.pathname || "/") === WIFI_AP_PATH;
  const { main } = buildShell({ variant: apSetupShell ? "ap-setup" : "full" });
  registerRoutes();
  configure({ outlet: main });
  installGlobalErrorHandlers();
  stopConnHealth = startConnHealthMonitor();
  markAppReady();
  void start();
  void refreshDeviceInfo().then(() => {
    hydrateFirmwareUpdateFromStorage();
    void runFirmwareUpdateDailyCheck();
  });
  deviceInfo.subscribe(() => {
    void rerender();
  });
  productModePioEnv.subscribe(() => {
    void (async () => {
      const path = currentPath();
      if ((path === "/actions" || path.startsWith("/actions/")) && !showActionsNav()) {
        await go("/");
        return;
      }
      // Keep settings form state (no remount); gates update via settingsLayout subscription.
      if (isSettingsPath(path)) return;
      await rerender();
    })();
  });

  if (!localeSubscribed) {
    localeSubscribed = true;
    localePref.subscribe(() => {
      void rerender();
    });
  }
}

let loginCleanup: (() => void) | null = null;

function mountLoginGate(returnTo: string, onSuccess: () => void): void {
  if (!isLoginPath(returnTo)) saveReturnTo(returnTo);
  syncLoginUrl();
  loginCleanup?.();
  loginCleanup = mountLoginPage({ onSuccess });
}

function redirectToLogin(returnTo: string): Promise<boolean> {
  if (loginPromise) return loginPromise;
  loginPromise = new Promise<boolean>((resolve) => {
    stopConnHealth();
    stopConnHealth = () => {};
    mountLoginGate(returnTo, () => {
      loginPromise = null;
      const dest = consumeReturnTo();
      startMainApp();
      void go(dest);
      resolve(true);
    });
  });
  return loginPromise;
}

function showLoginGate(): Promise<boolean> {
  const path = currentPath();
  const returnTo = isLoginPath(path) ? consumeReturnTo() : path;
  return redirectToLogin(returnTo || "/");
}

function mountLoginGateAfterBootstrap(): void {
  const returnTo = stripBase(location.pathname || "/");
  mountLoginGate(isLoginPath(returnTo) ? "/" : returnTo, () => {
    const dest = consumeReturnTo();
    startMainApp();
    void go(dest);
  });
}

async function resolveStartupAfterBootstrap(): Promise<void> {
  installRouteAuthGuard();
  registerAuthRequiredHandler(() => showLoginGate());

  if (isAuthExemptPath(location.pathname || "/")) {
    startMainApp();
    return;
  }

  if (!publicBootstrap.get().ready) {
    // Stay on index.html boot UI — avoid flashing the shell before auth is known.
    return;
  }

  if (requiresHttpAuthSession()) {
    mountLoginGateAfterBootstrap();
    return;
  }

  const boot = publicBootstrap.get();
  if (boot.httpAuthEnabled) {
    const ok = await probeApiSession();
    if (!ok) {
      clearSession();
      mountLoginGateAfterBootstrap();
      return;
    }
  }

  startMainApp();
}

void bootstrapUi().then(() => {
  void resolveStartupAfterBootstrap();
});
