import { describe, expect, it, vi } from "vitest";
import { buildEmptyState } from "../src/components/ui/emptyState";
import { buildPageHeader } from "../src/components/ui/pageHeader";
import { buildSectionCard } from "../src/components/ui/sectionCard";
import { buildSkeleton } from "../src/components/ui/skeleton";
import {
  buildRepresentationBanner,
  isRepresentationMode,
} from "../src/components/RepresentationBanner";
import { applyDocumentTitle } from "../src/utils/documentTitle";
import { wireTabsKeyboard } from "../src/utils/tabsKeyboard";
import { h } from "../src/utils/dom";

describe("ui primitives", () => {
  it("buildPageHeader renders title, description, and actions", () => {
    const action = h("button", {}, "Go");
    const header = buildPageHeader({
      title: "Settings",
      description: "  Device options  ",
      actions: [action],
      className: "extra",
    });
    expect(header.querySelector(".page-title")?.textContent).toBe("Settings");
    expect(header.querySelector(".page-header__desc")?.textContent).toContain("Device options");
    expect(header.querySelector(".page-header__actions")?.contains(action)).toBe(true);
    expect(header.className).toContain("extra");
  });

  it("buildEmptyState renders message and optional action", () => {
    const action = h("a", {}, "Retry");
    const el = buildEmptyState({ message: "No data", action });
    expect(el.querySelector("p")?.textContent).toBe("No data");
    expect(el.contains(action)).toBe(true);
  });

  it("buildSectionCard renders title, description, and children", () => {
    const child = h("p", {}, "child");
    const card = buildSectionCard({
      title: "Network",
      description: " Wi-Fi ",
      children: [child],
      className: "network",
    });
    expect(card.querySelector(".section__title")?.textContent).toBe("Network");
    expect(card.querySelector(".card__desc")?.textContent).toContain("Wi-Fi");
    expect(card.contains(child)).toBe(true);
    expect(card.className).toContain("network");
  });

  it("buildSkeleton renders line and block variants", () => {
    const sk = buildSkeleton(2, true);
    expect(sk.querySelectorAll(".skeleton").length).toBe(2);
    expect(sk.querySelector(".skeleton--block")).not.toBeNull();
  });

  it("buildRepresentationBanner shows status text", () => {
    const banner = buildRepresentationBanner();
    expect(banner.getAttribute("role")).toBe("status");
    expect(banner.className).toContain("representation-banner");
    expect(banner.textContent?.length).toBeGreaterThan(0);
  });

  it("isRepresentationMode follows Vite MODE", () => {
    vi.stubEnv("MODE", "representation");
    expect(isRepresentationMode()).toBe(true);
    vi.stubEnv("MODE", "test");
    expect(isRepresentationMode()).toBe(false);
  });

  it("applyDocumentTitle sets section and login titles", () => {
    applyDocumentTitle("/settings/general");
    expect(document.title).toContain("Settings");
    expect(document.title).toContain("General");
    applyDocumentTitle("/settings/metering");
    expect(document.title).toContain("Metering");
    applyDocumentTitle("/login");
    expect(document.title).toContain("Sign in");
    applyDocumentTitle("/unknown");
    expect(document.title.length).toBeGreaterThan(0);
  });

  it("wireTabsKeyboard moves selection with arrows and cleans up", () => {
    const tablist = document.createElement("div");
    tablist.setAttribute("role", "tablist");
    const tabs = [0, 1, 2].map((i) => {
      const tab = document.createElement("button");
      tab.setAttribute("role", "tab");
      tab.setAttribute("aria-selected", i === 0 ? "true" : "false");
      tab.tabIndex = i === 0 ? 0 : -1;
      tablist.append(tab);
      return tab;
    });
    const panels = [0, 1, 2].map((i) => {
      const panel = document.createElement("section");
      if (i !== 0) panel.setAttribute("hidden", "");
      tablist.append(panel);
      return panel;
    });
    const selected: number[] = [];
    const unwire = wireTabsKeyboard(tablist, tabs, panels, (index) => selected.push(index));

    tabs[1].click();
    expect(selected.at(-1)).toBe(1);
    expect(tabs[1].getAttribute("aria-selected")).toBe("true");
    expect(panels[1].hasAttribute("hidden")).toBe(false);

    tablist.dispatchEvent(new KeyboardEvent("keydown", { key: "ArrowRight", bubbles: true }));
    expect(tabs[2].getAttribute("aria-selected")).toBe("true");

    tablist.dispatchEvent(new KeyboardEvent("keydown", { key: "Home", bubbles: true }));
    expect(tabs[0].getAttribute("aria-selected")).toBe("true");

    tablist.dispatchEvent(new KeyboardEvent("keydown", { key: "End", bubbles: true }));
    expect(tabs[2].getAttribute("aria-selected")).toBe("true");

    unwire();
    selected.length = 0;
    tablist.dispatchEvent(new KeyboardEvent("keydown", { key: "ArrowLeft", bubbles: true }));
    expect(selected).toEqual([]);
  });
});
