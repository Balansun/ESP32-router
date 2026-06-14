import { api } from "../../api/client";
import { h } from "../../utils/dom";
import { toast } from "../../components/Toast";
import type { RouterConfig } from "../../api/types";
import { focusFirstInvalid } from "./formRows";

export type CardSaveStatus = "idle" | "dirty" | "saving" | "saved" | "error";

export interface SettingsCardAutosave {
  markDirty: () => void;
  flush: () => Promise<void>;
  isDirty: () => boolean;
  isPending: () => boolean;
}

export interface CardAutosaveLabels {
  pending: string;
  saving: string;
  saved: string;
  error: string;
}

export function attachCardAutosave<T extends object>(opts: {
  card: HTMLElement;
  signal: AbortSignal;
  collect: () => T | null;
  persist: (data: T) => Promise<void>;
  validate?: () => boolean;
  onSaved?: (data: T) => void;
  labels: CardAutosaveLabels;
  /** When true, debounce and persist on change (API CORS). Default: manual save only. */
  autoSave?: boolean;
  debounceMs?: number;
  /** Limit change listeners (default: all inputs/selects in card). */
  watchRoots?: HTMLElement[];
  onStateChange?: () => void;
}): SettingsCardAutosave {
  const autoSave = opts.autoSave ?? false;
  const debounceMs = opts.debounceMs ?? 700;
  const statusEl = h("p", {
    class: "card__save-status",
    hidden: true,
    "aria-live": "polite",
  }) as HTMLParagraphElement;
  opts.card.append(statusEl);

  let status: CardSaveStatus = "idle";
  let timer: ReturnType<typeof setTimeout> | undefined;
  let savedHideTimer: ReturnType<typeof setTimeout> | undefined;
  let inFlight: Promise<void> | null = null;

  function setStatus(next: CardSaveStatus) {
    status = next;
    opts.onStateChange?.();
    savedHideTimer && clearTimeout(savedHideTimer);
    if (next === "idle") {
      statusEl.hidden = true;
      statusEl.textContent = "";
      statusEl.className = "card__save-status";
      return;
    }
    statusEl.hidden = false;
    statusEl.className = `card__save-status card__save-status--${next}`;
    const text =
      next === "dirty"
        ? opts.labels.pending
        : next === "saving"
          ? opts.labels.saving
          : next === "saved"
            ? opts.labels.saved
            : opts.labels.error;
    statusEl.textContent = text;
    if (next === "saved") {
      savedHideTimer = setTimeout(() => {
        if (status === "saved") setStatus("idle");
      }, 2500);
    }
  }

  function scheduleSave() {
    if (opts.signal.aborted) return;
    setStatus("dirty");
    if (!autoSave) return;
    if (timer) clearTimeout(timer);
    timer = setTimeout(() => {
      timer = undefined;
      void flush();
    }, debounceMs);
  }

  async function flush(): Promise<void> {
    if (opts.signal.aborted) return;
    if (inFlight) {
      await inFlight;
      if (status !== "dirty" && status !== "error") return;
    } else if (status !== "dirty" && status !== "error") {
      return;
    }
    if (opts.validate && !opts.validate()) {
      setStatus("error");
      focusFirstInvalid(opts.card);
      return;
    }
    const data = opts.collect();
    if (!data || Object.keys(data).length === 0) {
      setStatus("idle");
      return;
    }
    setStatus("saving");
    inFlight = (async () => {
      try {
        await opts.persist(data);
        opts.onSaved?.(data);
        setStatus("saved");
      } catch (e) {
        console.error(e);
        setStatus("error");
        toast(opts.labels.error, "error");
      } finally {
        inFlight = null;
      }
    })();
    await inFlight;
  }

  function bindControl(el: HTMLInputElement | HTMLSelectElement) {
    if (el instanceof HTMLSelectElement) {
      el.addEventListener("change", scheduleSave);
      return;
    }
    el.addEventListener("input", scheduleSave);
    el.addEventListener("change", scheduleSave);
  }

  const roots = opts.watchRoots ?? [opts.card];
  for (const root of roots) {
    if (root instanceof HTMLInputElement || root instanceof HTMLSelectElement) {
      bindControl(root);
      continue;
    }
    for (const inputEl of root.querySelectorAll<HTMLInputElement>("input")) {
      bindControl(inputEl);
    }
    for (const selectEl of root.querySelectorAll<HTMLSelectElement>("select")) {
      bindControl(selectEl);
    }
  }

  opts.signal.addEventListener(
    "abort",
    () => {
      if (timer) clearTimeout(timer);
      if (savedHideTimer) clearTimeout(savedHideTimer);
    },
    { once: true },
  );

  return {
    markDirty: scheduleSave,
    flush,
    isDirty: () => status === "dirty" || status === "error",
    isPending: () => status === "saving" || (autoSave && !!timer),
  };
}

export function attachSettingsCardAutosave(opts: {
  card: HTMLElement;
  signal: AbortSignal;
  collect: () => Partial<RouterConfig> | null;
  persist?: (patch: Partial<RouterConfig>) => Promise<void>;
  validate?: () => boolean;
  onSaved?: (patch: Partial<RouterConfig>) => void;
  labels: CardAutosaveLabels;
  autoSave?: boolean;
  debounceMs?: number;
  watchRoots?: HTMLElement[];
  onStateChange?: () => void;
}): SettingsCardAutosave {
  const persist =
    opts.persist ??
    (async (patch) => {
      await api.patchConfig(patch, { signal: opts.signal, retry: 1 });
    });
  return attachCardAutosave({
    ...opts,
    persist,
  });
}
