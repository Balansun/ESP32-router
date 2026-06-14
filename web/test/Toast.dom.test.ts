import { afterEach, describe, expect, it, vi } from "vitest";
import { toast } from "../src/components/Toast";

describe("Toast", () => {
  afterEach(() => {
    document.body.replaceChildren();
  });

  it("info kind uses neutral toast class", () => {
    vi.useFakeTimers();
    toast("n", "info");
    const el = document.querySelector(".toast") as HTMLElement;
    expect(el.className).toBe("toast");
    el.remove();
    vi.useRealTimers();
  });

  it("shows and dismisses toast", () => {
    vi.useFakeTimers();
    toast("Hello", "success", 1000);
    expect(document.querySelector(".toast")).not.toBeNull();
    vi.advanceTimersByTime(1200);
    vi.advanceTimersByTime(250);
    vi.useRealTimers();
  });

  it("click removes toast early", () => {
    toast("Click me", "warn");
    const node = document.querySelector(".toast") as HTMLElement;
    node.click();
    expect(document.querySelector(".toast")).toBeNull();
  });
});
