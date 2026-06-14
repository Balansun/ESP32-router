import { describe, expect, it } from "vitest";
import { icon, type IconName } from "./icons";

const NAMES: IconName[] = [
  "home",
  "history",
  "actions",
  "settings",
  "diag",
  "sun",
  "moon",
  "auto",
  "plus",
  "minus",
  "edit",
  "close",
  "copy",
  "reboot",
  "save",
  "alert",
  "chip",
  "wifi",
  "more",
  "download",
  "upload",
];

describe("icons", () => {
  it("renders all icon names", () => {
    for (const name of NAMES) {
      const el = icon(name);
      expect(el.tagName.toLowerCase()).toBe("svg");
    }
  });

  it("supports aria label", () => {
    const el = icon("home", { ariaLabel: "Home" });
    expect(el.getAttribute("aria-label")).toBe("Home");
  });
});
