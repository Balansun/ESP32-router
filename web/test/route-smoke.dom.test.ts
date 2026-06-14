import { describe, expect, it, vi, beforeEach, afterEach } from "vitest";
import { poll } from "../src/state/store";

describe("route poll hygiene", () => {
  beforeEach(() => {
    vi.useFakeTimers();
  });

  afterEach(() => {
    vi.useRealTimers();
  });

  it("poll stops scheduling after stop()", async () => {
    const fn = vi.fn(async () => {});
    const handle = poll(fn, 1000, 5000, { immediate: true });
    await vi.advanceTimersByTimeAsync(0);
    expect(fn).toHaveBeenCalledTimes(1);
    handle.stop();
    await vi.advanceTimersByTimeAsync(5000);
    expect(fn).toHaveBeenCalledTimes(1);
  });

  it("poll skips work while document is hidden", async () => {
    const fn = vi.fn(async () => {});
    Object.defineProperty(document, "hidden", { configurable: true, value: true });
    const handle = poll(fn, 500, 5000, { immediate: true });
    await vi.advanceTimersByTimeAsync(0);
    expect(fn).not.toHaveBeenCalled();
    handle.stop();
    Object.defineProperty(document, "hidden", { configurable: true, value: false });
  });
});
