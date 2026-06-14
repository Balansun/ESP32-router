import { describe, expect, it, vi } from "vitest";
import { clear, h, svg } from "./dom";

describe("dom helpers", () => {
  it("h creates elements with class, style, and listeners", () => {
    const clicked = vi.fn();
    const el = h(
      "button",
      {
        class: "btn",
        style: "color:red",
        onClick: clicked,
      },
      "Hi",
    );
    expect(el.className).toBe("btn");
    el.dispatchEvent(new MouseEvent("click", { bubbles: true }));
    expect(clicked).toHaveBeenCalled();
  });

  it("h supports dataset on div", () => {
    const el = h("div", { dataset: { foo: "bar" } });
    expect(el.dataset.foo).toBe("bar");
  });

  it("svg builds namespaced elements", () => {
    const el = svg("svg", { width: 10 }, "t");
    expect(el.namespaceURI).toContain("svg");
    expect(el.getAttribute("width")).toBe("10");
    expect(el.textContent).toBe("t");
  });

  it("svg appends Node children", () => {
    const circle = document.createElementNS("http://www.w3.org/2000/svg", "circle");
    const el = svg("svg", { width: 10 }, circle);
    expect(el.querySelector("circle")).toBe(circle);
  });

  it("h skips null and false children", () => {
    const el = h("div", {}, "a", null, false, "b");
    expect(el.textContent).toBe("ab");
  });

  it("clear removes children", () => {
    const parent = h("div", {}, h("span", {}, "x"));
    clear(parent);
    expect(parent.childNodes.length).toBe(0);
  });
});
