import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import {
  attachCardAutosave,
  attachSettingsCardAutosave,
} from "../src/routes/settings/cardAutosave";

const apiMock = vi.hoisted(() => ({
  patchConfig: vi.fn(),
}));

vi.mock("../src/api/client", () => ({ api: apiMock }));
vi.mock("../src/components/Toast", () => ({ toast: vi.fn() }));

describe("cardAutosave", () => {
  beforeEach(() => {
    vi.useFakeTimers();
    vi.clearAllMocks();
    apiMock.patchConfig.mockResolvedValue(undefined);
  });

  afterEach(() => {
    vi.useRealTimers();
  });

  it("marks dirty on input without auto patch until flush", async () => {
    const card = document.createElement("section");
    const input = document.createElement("input");
    card.append(input);
    const ctrl = new AbortController();
    const autosave = attachSettingsCardAutosave({
      card,
      signal: ctrl.signal,
      collect: () => ({ router_name: "New" }),
      labels: {
        pending: "pending",
        saving: "saving",
        saved: "saved",
        error: "error",
      },
    });
    input.dispatchEvent(new Event("input", { bubbles: true }));
    autosave.markDirty();
    await vi.advanceTimersByTimeAsync(150);
    expect(apiMock.patchConfig).not.toHaveBeenCalled();
    expect(card.querySelector(".card__save-status--dirty")).not.toBeNull();
    await autosave.flush();
    expect(apiMock.patchConfig).toHaveBeenCalled();
    ctrl.abort();
    vi.useRealTimers();
  });

  it("validate failure sets error without patch", async () => {
    const card = document.createElement("section");
    const input = document.createElement("input");
    card.append(input);
    const ctrl = new AbortController();
    const autosave = attachSettingsCardAutosave({
      card,
      signal: ctrl.signal,
      collect: () => ({ router_name: "X" }),
      validate: () => false,
      labels: { pending: "p", saving: "s", saved: "ok", error: "err" },
      debounceMs: 50,
    });
    autosave.markDirty();
    await autosave.flush();
    expect(apiMock.patchConfig).not.toHaveBeenCalled();
    expect(card.querySelector(".card__save-status--error")).not.toBeNull();
    ctrl.abort();
    vi.useRealTimers();
  });

  it("empty collect resets to idle", async () => {
    const card = document.createElement("section");
    const ctrl = new AbortController();
    const autosave = attachSettingsCardAutosave({
      card,
      signal: ctrl.signal,
      collect: () => ({}),
      labels: { pending: "p", saving: "s", saved: "ok", error: "err" },
    });
    autosave.markDirty();
    await autosave.flush();
    expect(apiMock.patchConfig).not.toHaveBeenCalled();
    ctrl.abort();
    vi.useRealTimers();
  });

  it("patch failure shows error status", async () => {
    apiMock.patchConfig.mockRejectedValue(new Error("fail"));
    const card = document.createElement("section");
    const ctrl = new AbortController();
    const autosave = attachSettingsCardAutosave({
      card,
      signal: ctrl.signal,
      collect: () => ({ router_name: "Z" }),
      labels: { pending: "p", saving: "s", saved: "ok", error: "err" },
    });
    autosave.markDirty();
    await autosave.flush();
    expect(card.querySelector(".card__save-status--error")).not.toBeNull();
    ctrl.abort();
    vi.useRealTimers();
  });

  it("select change marks dirty without auto patch by default", async () => {
    const card = document.createElement("section");
    const select = document.createElement("select");
    const opt = document.createElement("option");
    opt.value = "a";
    select.append(opt);
    card.append(select);
    const ctrl = new AbortController();
    attachSettingsCardAutosave({
      card,
      signal: ctrl.signal,
      collect: () => ({ router_name: "Sel" }),
      labels: { pending: "p", saving: "s", saved: "ok", error: "err" },
    });
    select.dispatchEvent(new Event("change", { bubbles: true }));
    await vi.advanceTimersByTimeAsync(100);
    expect(apiMock.patchConfig).not.toHaveBeenCalled();
    expect(card.querySelector(".card__save-status--dirty")).not.toBeNull();
    ctrl.abort();
    vi.useRealTimers();
  });

  it("autoSave debounces patch on change", async () => {
    const card = document.createElement("section");
    const select = document.createElement("select");
    const opt = document.createElement("option");
    opt.value = "a";
    select.append(opt);
    card.append(select);
    const ctrl = new AbortController();
    attachSettingsCardAutosave({
      card,
      signal: ctrl.signal,
      collect: () => ({ router_name: "Sel" }),
      labels: { pending: "p", saving: "s", saved: "ok", error: "err" },
      autoSave: true,
      debounceMs: 80,
    });
    select.dispatchEvent(new Event("change", { bubbles: true }));
    await vi.advanceTimersByTimeAsync(100);
    expect(apiMock.patchConfig).toHaveBeenCalled();
    ctrl.abort();
    vi.useRealTimers();
  });

  it("flush no-ops when signal already aborted", async () => {
    const card = document.createElement("section");
    const ctrl = new AbortController();
    const autosave = attachSettingsCardAutosave({
      card,
      signal: ctrl.signal,
      collect: () => ({ router_name: "Aborted" }),
      labels: { pending: "p", saving: "s", saved: "ok", error: "err" },
    });
    autosave.markDirty();
    ctrl.abort();
    await autosave.flush();
    expect(apiMock.patchConfig).not.toHaveBeenCalled();
    vi.useRealTimers();
  });

  it("retries patch after error when flush called again", async () => {
    apiMock.patchConfig.mockRejectedValueOnce(new Error("fail")).mockResolvedValueOnce(undefined);
    const card = document.createElement("section");
    const ctrl = new AbortController();
    const autosave = attachSettingsCardAutosave({
      card,
      signal: ctrl.signal,
      collect: () => ({ router_name: "Retry" }),
      labels: { pending: "p", saving: "s", saved: "ok", error: "err" },
    });
    autosave.markDirty();
    await autosave.flush();
    await autosave.flush();
    expect(apiMock.patchConfig).toHaveBeenCalledTimes(2);
    ctrl.abort();
  });

  it("second flush exits after in-flight save when no longer dirty", async () => {
    apiMock.patchConfig.mockResolvedValue(undefined);
    const card = document.createElement("section");
    const ctrl = new AbortController();
    const autosave = attachSettingsCardAutosave({
      card,
      signal: ctrl.signal,
      collect: () => ({ router_name: "Once" }),
      labels: { pending: "p", saving: "s", saved: "ok", error: "err" },
    });
    autosave.markDirty();
    const first = autosave.flush();
    await Promise.resolve();
    const second = autosave.flush();
    await first;
    await second;
    expect(apiMock.patchConfig).toHaveBeenCalledTimes(1);
    ctrl.abort();
  });

  it("recursively flushes when still dirty/error after awaiting in-flight save", async () => {
    apiMock.patchConfig.mockRejectedValueOnce(new Error("first fail")).mockResolvedValueOnce(undefined);
    const card = document.createElement("section");
    const input = document.createElement("input");
    card.append(input);
    const ctrl = new AbortController();
    const autosave = attachSettingsCardAutosave({
      card,
      signal: ctrl.signal,
      collect: () => ({ router_name: "Retry" }),
      labels: { pending: "p", saving: "s", saved: "ok", error: "err" },
    });
    autosave.markDirty();
    const p1 = autosave.flush();
    await Promise.resolve();
    input.dispatchEvent(new Event("input", { bubbles: true }));
    const p2 = autosave.flush();
    await p1;
    await p2;
    expect(apiMock.patchConfig).toHaveBeenCalledTimes(2);
    ctrl.abort();
    vi.useRealTimers();
  });

  it("waits for in-flight save before second flush", async () => {
    let resolveFirst: () => void = () => {};
    apiMock.patchConfig.mockImplementationOnce(
      () =>
        new Promise<void>((resolve) => {
          resolveFirst = resolve;
        }),
    );
    const card = document.createElement("section");
    const ctrl = new AbortController();
    const autosave = attachSettingsCardAutosave({
      card,
      signal: ctrl.signal,
      collect: () => ({ router_name: "A" }),
      labels: { pending: "p", saving: "s", saved: "ok", error: "err" },
    });
    autosave.markDirty();
    const first = autosave.flush();
    autosave.markDirty();
    const second = autosave.flush();
    resolveFirst();
    await first;
    await second;
    expect(apiMock.patchConfig).toHaveBeenCalledTimes(1);
    ctrl.abort();
    vi.useRealTimers();
  });

  it("collect null resets idle without patch", async () => {
    const card = document.createElement("section");
    const ctrl = new AbortController();
    const autosave = attachSettingsCardAutosave({
      card,
      signal: ctrl.signal,
      collect: () => null,
      labels: { pending: "p", saving: "s", saved: "ok", error: "err" },
    });
    autosave.markDirty();
    await autosave.flush();
    expect(apiMock.patchConfig).not.toHaveBeenCalled();
    ctrl.abort();
    vi.useRealTimers();
  });

  it("onStateChange fires on status transitions", async () => {
    const card = document.createElement("section");
    const ctrl = new AbortController();
    const onStateChange = vi.fn();
    const autosave = attachSettingsCardAutosave({
      card,
      signal: ctrl.signal,
      collect: () => ({ router_name: "S" }),
      labels: { pending: "p", saving: "s", saved: "ok", error: "err" },
      onStateChange,
    });
    autosave.markDirty();
    await autosave.flush();
    expect(onStateChange).toHaveBeenCalled();
    expect(autosave.isDirty()).toBe(false);
    ctrl.abort();
    vi.useRealTimers();
  });

  it("flush no-ops when not dirty", async () => {
    const card = document.createElement("section");
    const ctrl = new AbortController();
    const autosave = attachSettingsCardAutosave({
      card,
      signal: ctrl.signal,
      collect: () => ({ router_name: "Idle" }),
      labels: { pending: "p", saving: "s", saved: "ok", error: "err" },
    });
    await autosave.flush();
    expect(apiMock.patchConfig).not.toHaveBeenCalled();
    ctrl.abort();
    vi.useRealTimers();
  });

  it("onSaved receives patch after success", async () => {
    const card = document.createElement("section");
    const ctrl = new AbortController();
    const onSaved = vi.fn();
    const autosave = attachSettingsCardAutosave({
      card,
      signal: ctrl.signal,
      collect: () => ({ router_name: "Saved" }),
      labels: { pending: "p", saving: "s", saved: "ok", error: "err" },
      onSaved,
    });
    autosave.markDirty();
    await autosave.flush();
    expect(onSaved).toHaveBeenCalledWith({ router_name: "Saved" });
    ctrl.abort();
    vi.useRealTimers();
  });

  it("isPending false while dirty in manual mode", () => {
    const card = document.createElement("section");
    const ctrl = new AbortController();
    const autosave = attachSettingsCardAutosave({
      card,
      signal: ctrl.signal,
      collect: () => ({ router_name: "P" }),
      labels: { pending: "p", saving: "s", saved: "ok", error: "err" },
    });
    autosave.markDirty();
    expect(autosave.isPending()).toBe(false);
    expect(autosave.isDirty()).toBe(true);
    ctrl.abort();
    vi.useRealTimers();
  });

  it("isPending true while debounce timer is active with autoSave", () => {
    const card = document.createElement("section");
    const ctrl = new AbortController();
    const autosave = attachSettingsCardAutosave({
      card,
      signal: ctrl.signal,
      collect: () => ({ router_name: "P" }),
      labels: { pending: "p", saving: "s", saved: "ok", error: "err" },
      autoSave: true,
      debounceMs: 500,
    });
    autosave.markDirty();
    expect(autosave.isPending()).toBe(true);
    ctrl.abort();
    vi.useRealTimers();
  });

  it("scheduleSave no-ops after signal aborted", async () => {
    const card = document.createElement("section");
    const ctrl = new AbortController();
    const autosave = attachSettingsCardAutosave({
      card,
      signal: ctrl.signal,
      collect: () => ({ router_name: "A" }),
      labels: { pending: "p", saving: "s", saved: "ok", error: "err" },
      debounceMs: 50,
    });
    ctrl.abort();
    autosave.markDirty();
    await vi.advanceTimersByTimeAsync(100);
    expect(apiMock.patchConfig).not.toHaveBeenCalled();
    vi.useRealTimers();
  });

  it("binds listeners when watchRoots is a single control", async () => {
    const card = document.createElement("section");
    const input = document.createElement("input");
    input.type = "checkbox";
    card.append(input);
    const ctrl = new AbortController();
    const autosave = attachSettingsCardAutosave({
      card,
      signal: ctrl.signal,
      watchRoots: [input],
      collect: () => ({ http_cors_enabled: input.checked }),
      labels: { pending: "p", saving: "s", saved: "ok", error: "err" },
    });
    input.checked = true;
    input.dispatchEvent(new Event("change", { bubbles: true }));
    await vi.advanceTimersByTimeAsync(150);
    expect(apiMock.patchConfig).not.toHaveBeenCalled();
    await autosave.flush();
    expect(apiMock.patchConfig).toHaveBeenCalledWith(
      { http_cors_enabled: true },
      expect.anything(),
    );
    ctrl.abort();
    vi.useRealTimers();
  });

  it("attachCardAutosave uses custom persist on flush", async () => {
    vi.useFakeTimers();
    const card = document.createElement("section");
    const ctrl = new AbortController();
    const persist = vi.fn().mockResolvedValue(undefined);
    const input = document.createElement("input");
    input.value = "Europe/Paris";
    card.append(input);
    const autosave = attachCardAutosave({
      card,
      signal: ctrl.signal,
      watchRoots: [input],
      collect: () => ({ tz: input.value, ntp1: "", ntp2: "" }),
      persist,
      labels: { pending: "p", saving: "s", saved: "ok", error: "err" },
    });
    input.value = "UTC";
    input.dispatchEvent(new Event("input", { bubbles: true }));
    await vi.advanceTimersByTimeAsync(800);
    expect(persist).not.toHaveBeenCalled();
    await autosave.flush();
    expect(persist).toHaveBeenCalledWith({ tz: "UTC", ntp1: "", ntp2: "" });
    ctrl.abort();
    vi.useRealTimers();
  });

  it("saved status hides after timeout", async () => {
    const card = document.createElement("section");
    const ctrl = new AbortController();
    const autosave = attachSettingsCardAutosave({
      card,
      signal: ctrl.signal,
      collect: () => ({ router_name: "Hide" }),
      labels: { pending: "p", saving: "s", saved: "ok", error: "err" },
    });
    autosave.markDirty();
    await autosave.flush();
    expect(card.querySelector(".card__save-status--saved")).not.toBeNull();
    await vi.advanceTimersByTimeAsync(2600);
    expect(card.querySelector(".card__save-status")?.hidden).toBe(true);
    ctrl.abort();
    vi.useRealTimers();
  });
});
