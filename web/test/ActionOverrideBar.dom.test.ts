import { describe, expect, it, vi } from "vitest";
import { buildOverrideControls, overrideMap } from "../src/components/ActionOverrideBar";

describe("ActionOverrideBar", () => {
  it("overrideMap indexes by action index", () => {
    const m = overrideMap([
      { index: 0, state: "on" },
      { index: 2, state: "off" },
    ] as never[]);
    expect(m.get(0)?.state).toBe("on");
    expect(m.get(1)).toBeUndefined();
  });

  it("buildOverrideControls fires onSet and triac full", () => {
    const onSet = vi.fn();
    const onTriac = vi.fn();
    const el = buildOverrideControls({
      index: 0,
      active: { index: 0, state: "auto" },
      onSet,
      onTriacFull: onTriac,
    });
    const buttons = el.querySelectorAll("button");
    (buttons[1] as HTMLButtonElement).click();
    expect(onSet).toHaveBeenCalledWith("on");
    const fullBtn = Array.from(buttons).find((b) =>
      b.textContent?.toLowerCase().includes("100"),
    ) as HTMLButtonElement | undefined;
    fullBtn?.click();
    expect(onTriac).toHaveBeenCalled();
  });

  it("shows active badge when override is not auto", () => {
    const el = buildOverrideControls({
      index: 0,
      active: { index: 0, state: "on" },
      onSet: () => {},
    });
    expect(el.querySelector(".override-bar__badge")).not.toBeNull();
  });

  it("omit triac-full on non-triac index", () => {
    const el = buildOverrideControls({
      index: 2,
      active: { index: 2, state: "auto" },
      onSet: () => {},
      onTriacFull: vi.fn(),
    });
    expect(
      Array.from(el.querySelectorAll("button")).some((b) =>
        (b.textContent ?? "").includes("100"),
      ),
    ).toBe(false);
  });

  it("triac full button highlights when override is triac_fixed at 100%", () => {
    const el = buildOverrideControls({
      index: 0,
      active: { index: 0, state: "triac_fixed", triac_open_percent: 100 },
      onSet: () => {},
      onTriacFull: vi.fn(),
    });
    const fullBtn = Array.from(el.querySelectorAll("button")).find((b) =>
      b.textContent?.toLowerCase().includes("100"),
    )!;
    expect(fullBtn.className).toContain("override-bar__btn--active");
  });

  it("overrideMap handles undefined summary", () => {
    expect(overrideMap(undefined).size).toBe(0);
  });

  it("off and auto buttons call onSet", () => {
    const onSet = vi.fn();
    const el = buildOverrideControls({
      index: 1,
      active: { index: 1, state: "off" },
      onSet,
    });
    const buttons = Array.from(el.querySelectorAll("button"));
    (buttons[0] as HTMLButtonElement).click();
    expect(onSet).toHaveBeenCalledWith("auto");
    (buttons[2] as HTMLButtonElement).click();
    expect(onSet).toHaveBeenCalledWith("off");
  });

  it("triac full not active below 100 percent", () => {
    const el = buildOverrideControls({
      index: 0,
      active: { index: 0, state: "triac_fixed", triac_open_percent: 50 },
      onSet: () => {},
      onTriacFull: vi.fn(),
    });
    const fullBtn = Array.from(el.querySelectorAll("button")).find((b) =>
      b.textContent?.toLowerCase().includes("100"),
    )!;
    expect(fullBtn.className).not.toContain("override-bar__btn--active");
  });

  it("triac full not active when triac_fixed has no percent", () => {
    const el = buildOverrideControls({
      index: 0,
      active: { index: 0, state: "triac_fixed" },
      onSet: () => {},
      onTriacFull: vi.fn(),
    });
    const fullBtn = Array.from(el.querySelectorAll("button")).find((b) =>
      b.textContent?.toLowerCase().includes("100"),
    )!;
    expect(fullBtn.className).not.toContain("override-bar__btn--active");
  });

  it("defaults to auto when active override missing", () => {
    const el = buildOverrideControls({ index: 0, onSet: () => {} });
    expect(el.querySelector('[aria-pressed="true"]')).not.toBeNull();
  });

  it("disables all buttons when safety lockout is active", () => {
    const onSet = vi.fn();
    const onTriac = vi.fn();
    const el = buildOverrideControls({
      index: 0,
      active: { index: 0, state: "auto" },
      disabled: true,
      onSet,
      onTriacFull: onTriac,
    });
    for (const btn of el.querySelectorAll("button")) {
      expect((btn as HTMLButtonElement).disabled).toBe(true);
    }
    (el.querySelector("button") as HTMLButtonElement).click();
    expect(onSet).not.toHaveBeenCalled();
    expect(onTriac).not.toHaveBeenCalled();
  });
});
