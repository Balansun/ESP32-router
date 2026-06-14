// Tiny pub/sub store with a shared polling clock.

import type { DeviceCapabilities } from "../api/types";
import { reportPollFailure, reportPollSuccess } from "./connStatus";

export interface PollHandle {
  stop(): void;
}

export interface PollOptions {
  /** Run first tick immediately (default true). */
  immediate?: boolean;
  /** Delay before the first tick (ms). */
  startDelayMs?: number;
}

type Listener<T> = (v: T, prev: T) => void;

export interface Signal<T> {
  get(): T;
  set(updater: T | ((prev: T) => T)): void;
  subscribe(listener: Listener<T>): () => void;
}

export function signal<T>(initial: T): Signal<T> {
  let value = initial;
  const listeners = new Set<Listener<T>>();
  return {
    get: () => value,
    set(updater) {
      const next =
        typeof updater === "function"
          ? (updater as (prev: T) => T)(value)
          : updater;
      if (Object.is(next, value)) return;
      const prev = value;
      value = next;
      for (const l of listeners) l(value, prev);
    },
    subscribe(listener) {
      listeners.add(listener);
      return () => listeners.delete(listener);
    },
  };
}

export type ConnState = "ok" | "loading" | "err";

export const conn = signal<ConnState>("loading");
export const offline = signal<boolean>(
  typeof navigator !== "undefined" ? !navigator.onLine : false,
);

if (typeof window !== "undefined") {
  window.addEventListener("online", () => offline.set(false));
  window.addEventListener("offline", () => offline.set(true));
}

function isAbortError(e: unknown): boolean {
  return (
    (e as DOMException)?.name === "AbortError" ||
    (e instanceof Error && e.message === "Aborted")
  );
}

export function poll(
  fn: () => Promise<void>,
  intervalMs: number,
  errorBackoffMs = 5000,
  options: boolean | PollOptions = true,
): PollHandle {
  const opts: PollOptions =
    typeof options === "boolean" ? { immediate: options } : options;
  const immediate = opts.immediate ?? true;
  const startDelayMs = opts.startDelayMs ?? 0;

  let stopped = false;
  let timer: number | undefined;
  const run = async () => {
    if (stopped) return;
    if (typeof document !== "undefined" && document.hidden) {
      if (!stopped) timer = window.setTimeout(run, intervalMs);
      return;
    }
    try {
      await fn();
      reportPollSuccess();
      if (!stopped) timer = window.setTimeout(run, intervalMs);
    } catch (e) {
      if (stopped || isAbortError(e)) return;
      reportPollFailure();
      if (!stopped) timer = window.setTimeout(run, errorBackoffMs);
    }
  };
  const scheduleFirst = () => {
    if (startDelayMs > 0) timer = window.setTimeout(run, startDelayMs);
    else if (immediate) queueMicrotask(run);
    else timer = window.setTimeout(run, intervalMs);
  };
  scheduleFirst();
  return {
    stop() {
      stopped = true;
      if (timer !== undefined) clearTimeout(timer);
    },
  };
}

// Theme handling
export type ThemePref = "auto" | "light" | "dark";

const THEME_KEY = "solar.theme";

export const themePref = signal<ThemePref>(
  (typeof localStorage !== "undefined" &&
    (localStorage.getItem(THEME_KEY) as ThemePref | null)) ||
    "auto",
);

themePref.subscribe((v) => {
  try {
    localStorage.setItem(THEME_KEY, v);
  } catch {
    // ignore quota / private mode errors
  }
  applyTheme(v);
});

export function applyTheme(pref: ThemePref) {
  const root = document.documentElement;
  if (pref === "auto") {
    root.removeAttribute("data-theme");
  } else {
    root.setAttribute("data-theme", pref);
  }
}

export type LocalePref = "en" | "fr";

const LOCALE_KEY = "solar.locale";

function readStoredLocale(): LocalePref {
  if (typeof localStorage === "undefined") return "en";
  const raw = localStorage.getItem(LOCALE_KEY);
  if (raw === "fr" || raw === "en") return raw;
  if (
    typeof navigator !== "undefined" &&
    navigator.language?.toLowerCase().startsWith("fr")
  ) {
    return "fr";
  }
  return "en";
}

export const localePref = signal<LocalePref>(readStoredLocale());

function applyLocale(lang: LocalePref) {
  if (typeof document !== "undefined") {
    document.documentElement.lang = lang;
  }
}

applyLocale(localePref.get());

localePref.subscribe((v) => {
  try {
    localStorage.setItem(LOCALE_KEY, v);
  } catch {
    // ignore quota / private mode errors
  }
  applyLocale(v);
});

/** From GET /api/v1/public — drives HTTP auth prompt visibility without login. */
export const publicBootstrap = signal<{
  ready: boolean;
  httpAuthEnabled: boolean;
  wifi?: { mode: "ap" | "sta"; connected: boolean; setup_ap?: boolean };
}>({ ready: false, httpAuthEnabled: false });

// Cache of device info (router name, probe names, firmware version, …)
export const deviceInfo = signal<{
  router_name: string;
  firmware_version: string;
  probe_house_name: string;
  probe_second_name: string;
  temperature_label: string;
  capabilities?: DeviceCapabilities;
} | null>(null);
