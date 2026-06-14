import { describe, expect, it } from "vitest";
import { buildSwitch, numberField, textField } from "./formControls";

describe("formControls DOM", () => {
  it("buildSwitch reflects checked state and fires onChange", () => {
    let value = false;
    const el = buildSwitch(false, "Enable", (v) => {
      value = v;
    });
    const input = el.querySelector('input[type="checkbox"]') as HTMLInputElement;
    expect(input.checked).toBe(false);
    input.checked = true;
    input.dispatchEvent(new Event("change", { bubbles: true }));
    expect(value).toBe(true);
  });

  it("textField updates via input event", () => {
    let value = "init";
    const el = textField("Label", "hint", "init", (v) => {
      value = v;
    });
    const input = el.querySelector("input") as HTMLInputElement;
    input.value = "changed";
    input.dispatchEvent(new Event("input", { bubbles: true }));
    expect(value).toBe("changed");
  });

  it("numberField renders with field help when help opts set", () => {
    const el = numberField("Cap", "0", () => {}, "hint", {
      helpScope: "actions",
      helpKey: "action_daily_cap_wh",
    });
    expect(el.querySelector(".field-help")).not.toBeNull();
  });
});
