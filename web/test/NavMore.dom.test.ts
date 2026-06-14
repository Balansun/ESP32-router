import { describe, expect, it, vi } from "vitest";
import { isMoreRoute, openNavMoreSheet } from "../src/components/NavMore";

const goMock = vi.hoisted(() => vi.fn());
vi.mock("../src/router", () => ({ go: goMock }));

describe("NavMore sheet", () => {
  it("isMoreRoute matches diag and subpaths", () => {
    expect(isMoreRoute("/diag")).toBe(true);
    expect(isMoreRoute("/diag/")).toBe(true);
    expect(isMoreRoute("/firmware/extra")).toBe(true);
    expect(isMoreRoute("/")).toBe(false);
    expect(isMoreRoute("/settings")).toBe(false);
  });

  it("openNavMoreSheet renders navigation buttons", () => {
    openNavMoreSheet();
    expect(document.querySelector(".nav-more")).not.toBeNull();
    expect(document.querySelectorAll(".nav-more__group").length).toBe(3);
    expect(document.querySelectorAll(".nav-more__item").length).toBe(5);
  });

  it("nav item click navigates and closes dialog", () => {
    openNavMoreSheet();
    const btn = document.querySelector(".nav-more__item") as HTMLButtonElement;
    btn.click();
    expect(goMock).toHaveBeenCalledWith("/wifi/station");
  });
});
