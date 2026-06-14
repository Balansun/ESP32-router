// Tiny client-side router with history API + hash fallback.
// Routes are registered upfront; navigation is done via go() or by clicking
// `<a data-route href="/...">` links handled globally.

import { confirmDiscardChanges } from "./navigationGuard";
import { normalizeLogicalPath, stripBase, toBrowserPath } from "./paths";
import { isSettingsPath, normalizeSettingsRedirect } from "./routes/settings/settingsPaths";
import { tryUpdateSettingsLayout } from "./routes/settings/settingsRouterBridge";

export interface RouteCtx {
  path: string;
  /** Container the route should render into. Always cleared before mount. */
  outlet: HTMLElement;
  /** Aborts when the user navigates away — use for fetch / poll cleanup. */
  signal: AbortSignal;
}

export type RouteCleanup = () => void;
export type RouteHandler = (
  ctx: RouteCtx,
) => void | RouteCleanup | Promise<void | RouteCleanup>;

interface Route {
  path: string;
  handler: RouteHandler;
  layout?: string;
}

let outlet: HTMLElement | null = null;
const routes: Route[] = [];
const navListeners = new Set<(path: string) => void>();
let activeAbort: AbortController | null = null;
let activeCleanup: RouteCleanup | null = null;
let activeLayoutId: string | null = null;
let useHash = false;
let pendingNav: Promise<void> = Promise.resolve();

export type RouteAuthGuard = (targetPath: string) => boolean;
let routeAuthGuard: RouteAuthGuard | null = null;

export function setRouteAuthGuard(guard: RouteAuthGuard | null): void {
  routeAuthGuard = guard;
}

function allowRoute(targetPath: string): boolean {
  if (!routeAuthGuard) return true;
  return routeAuthGuard(targetPath);
}

/** Hash routing for file:// static SPA copies. */
function defaultUseHashRouting(): boolean {
  if (typeof location === "undefined") return false;
  return location.protocol === "file:";
}

export function configure(opts: { outlet: HTMLElement; useHash?: boolean }) {
  outlet = opts.outlet;
  useHash = opts.useHash ?? defaultUseHashRouting();
}

export function register(
  path: string,
  handler: RouteHandler,
  opts?: { layout?: string },
) {
  routes.push({ path, handler, layout: opts?.layout });
}

export function onNav(cb: (path: string) => void): () => void {
  navListeners.add(cb);
  return () => navListeners.delete(cb);
}

export function currentPath(): string {
  if (useHash) {
    const h = location.hash.replace(/^#/, "");
    return normalizeLogicalPath(h ? `/${h.replace(/^\//, "")}` : "/");
  }
  return normalizeLogicalPath(location.pathname || "/");
}

export async function go(path: string, opts: { replace?: boolean } = {}) {
  const logical = path.startsWith("/") ? path : `/${path}`;
  const targetPath = stripBase(logical);
  if (!allowRoute(targetPath)) return;
  if (targetPath !== currentPath()) {
    const ok = await confirmDiscardChanges();
    if (!ok) return;
  }
  if (useHash) {
    const key = stripBase(logical);
    const url = `#${key}`;
    if (opts.replace) location.replace(url);
    else location.hash = key;
  } else {
    const target = toBrowserPath(logical);
    if (opts.replace) history.replaceState({}, "", target);
    else history.pushState({}, "", target);
    await renderOutlet();
  }
}

function findRoute(path: string): Route {
  const exact = routes.find((r) => r.path === path);
  if (exact) return exact;
  if (isSettingsPath(path)) {
    const settings = routes.find((r) => r.layout === "settings");
    if (settings) return settings;
  }
  return routes.find((r) => r.path === "*") || routes[0];
}

function runCleanup() {
  if (activeCleanup) {
    activeCleanup();
    activeCleanup = null;
  }
  activeLayoutId = null;
}

async function renderOutlet() {
  if (!outlet) return;
  let path = currentPath();
  if (!allowRoute(path)) return;
  const settingsRedirect = normalizeSettingsRedirect(path);
  if (settingsRedirect) {
    await go(settingsRedirect, { replace: true });
    return;
  }
  const route = findRoute(path);
  const layoutId = route.layout ?? null;

  if (layoutId && activeLayoutId === layoutId && tryUpdateSettingsLayout(path)) {
    for (const cb of navListeners) cb(path);
    return;
  }

  runCleanup();
  activeLayoutId = null;
  if (activeAbort) activeAbort.abort();
  activeAbort = new AbortController();
  outlet.replaceChildren();
  outlet.removeAttribute("aria-busy");
  for (const cb of navListeners) cb(path);
  try {
    const result = await route.handler({
      path,
      outlet,
      signal: activeAbort.signal,
    });
    if (typeof result === "function") activeCleanup = result;
    activeLayoutId = layoutId;
  } catch (e) {
    if ((e as DOMException)?.name === "AbortError") return;
    console.error("Route render failed", e);
  }
}

/** Re-run the current route (e.g. after locale change). */
export async function rerender() {
  await renderOutlet();
}

/** Remount the active route even when staying on the same settings layout path. */
export async function forceRerender() {
  runCleanup();
  activeLayoutId = null;
  await renderOutlet();
}

async function handleNavClick(ev: MouseEvent) {
  if (ev.defaultPrevented || ev.button !== 0 || ev.metaKey || ev.ctrlKey ||
    ev.shiftKey || ev.altKey) return;
  const a = (ev.target as Element | null)?.closest?.("a[data-route]");
  if (!a) return;
  const href = (a as HTMLAnchorElement).getAttribute("href");
  if (!href || href.startsWith("http")) return;
  ev.preventDefault();
  pendingNav = pendingNav.then(() => go(href));
  await pendingNav;
}

function bindGlobalNav() {
  document.addEventListener("click", (ev) => {
    void handleNavClick(ev);
  });
  if (useHash) {
    window.addEventListener("hashchange", () => {
      pendingNav = pendingNav.then(() => renderOutlet());
    });
  } else {
    window.addEventListener("popstate", () => {
      pendingNav = pendingNav.then(async () => {
        const ok = await confirmDiscardChanges();
        if (!ok) {
          history.go(1);
          return;
        }
        await renderOutlet();
      });
    });
  }
}

export async function start() {
  bindGlobalNav();
  await renderOutlet();
}
