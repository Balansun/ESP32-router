import { afterEach, describe, expect, it, vi } from "vitest";
import { h } from "../src/utils/dom";
import { openDialog } from "../src/components/Dialog";

const nativeShowModal = HTMLDialogElement.prototype.showModal;

describe("Dialog", () => {
  afterEach(() => {
    document.body.replaceChildren();
    HTMLDialogElement.prototype.showModal = nativeShowModal;
  });

  it("openDialog renders and closes", () => {
    const onClick = vi.fn();
    const { close } = openDialog({
      title: "Test",
      body: h("p", {}, "Body"),
      actions: [{ label: "OK", onClick }],
    });
    const dlg = document.querySelector("dialog");
    expect(dlg).not.toBeNull();
    close();
    expect(onClick).not.toHaveBeenCalled();
  });

  it("action button runs onClick and closes", async () => {
    const onClick = vi.fn();
    openDialog({
      title: "T",
      body: "text body",
      closeOnBackdrop: false,
      actions: [{ label: "Go", kind: "primary", onClick }],
    });
    const btn = document.querySelector(".btn--primary") as HTMLButtonElement;
    btn.click();
    await Promise.resolve();
    expect(onClick).toHaveBeenCalled();
  });

  it("backdrop click closes when enabled", () => {
    openDialog({ title: "T", body: "x", closeOnBackdrop: true });
    const dlg = document.querySelector("dialog")!;
    dlg.dispatchEvent(new MouseEvent("click", { bubbles: true }));
    expect(dlg.open).toBe(false);
  });

  it("header close button invokes close handler", () => {
    const { close } = openDialog({ title: "T", body: "x", closeOnBackdrop: false });
    const closeBtn = document.querySelector(".icon-btn") as HTMLButtonElement;
    closeBtn.click();
    const dlg = document.querySelector("dialog") as HTMLDialogElement;
    expect(dlg.open).toBe(false);
    close();
  });

  it("default action kind omits modifier class", () => {
    openDialog({
      title: "T",
      body: "x",
      closeOnBackdrop: false,
      actions: [{ label: "Plain", kind: "default", onClick: () => {} }],
    });
    const btn = document.querySelector("dialog .btn:not([class*='--'])") as HTMLButtonElement;
    expect(btn.className).toBe("btn");
  });

  it("uses open attribute when showModal is unavailable", () => {
    const setAttr = vi.spyOn(HTMLDialogElement.prototype, "setAttribute");
    HTMLDialogElement.prototype.showModal = undefined as typeof nativeShowModal;
    openDialog({ title: "T", body: "x", closeOnBackdrop: false });
    expect(setAttr).toHaveBeenCalledWith("open", "");
    setAttr.mockRestore();
  });

  it("runs sync onClick in the same turn as the click (no await microtask)", () => {
    const order: string[] = [];
    openDialog({
      title: "T",
      body: "x",
      closeOnBackdrop: false,
      actions: [
        {
          label: "Copy",
          kind: "primary",
          closeOnClick: false,
          onClick: () => {
            order.push("handler");
          },
        },
      ],
    });
    const btn = document.querySelector(".btn--primary") as HTMLButtonElement;
    btn.click();
    order.push("after-click");
    expect(order).toEqual(["handler", "after-click"]);
  });

  it("awaits async onClick before closing", async () => {
    const order: string[] = [];
    openDialog({
      title: "T",
      body: "x",
      closeOnBackdrop: false,
      actions: [
        {
          label: "Go",
          kind: "primary",
          onClick: async () => {
            await Promise.resolve();
            order.push("async-done");
          },
        },
      ],
    });
    const btn = document.querySelector(".btn--primary") as HTMLButtonElement;
    btn.click();
    order.push("after-click");
    await Promise.resolve();
    expect(order).toEqual(["after-click", "async-done"]);
  });

  it("logs async onClick rejection without closing when closeOnClick false", async () => {
    const errSpy = vi.spyOn(console, "error").mockImplementation(() => {});
    openDialog({
      title: "T",
      body: "x",
      closeOnBackdrop: false,
      actions: [
        {
          label: "Bad",
          kind: "danger",
          closeOnClick: false,
          onClick: async () => {
            throw new Error("async-boom");
          },
        },
      ],
    });
    const btn = document.querySelector(".btn--danger") as HTMLButtonElement;
    btn.click();
    await new Promise((r) => setTimeout(r, 0));
    expect(document.querySelector("dialog")?.open).toBe(true);
    expect(errSpy).toHaveBeenCalled();
    errSpy.mockRestore();
  });

  it("logs action errors without closing when closeOnClick false", async () => {
    const errSpy = vi.spyOn(console, "error").mockImplementation(() => {});
    openDialog({
      title: "T",
      body: "x",
      closeOnBackdrop: false,
      actions: [
        {
          label: "Bad",
          kind: "danger",
          closeOnClick: false,
          onClick: () => {
            throw new Error("boom");
          },
        },
      ],
    });
    const btn = document.querySelector(".btn--danger") as HTMLButtonElement;
    btn.click();
    await Promise.resolve();
    expect(errSpy).toHaveBeenCalled();
    errSpy.mockRestore();
  });

});
