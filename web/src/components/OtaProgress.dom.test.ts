import { describe, expect, it } from "vitest";
import { createOtaProgress } from "./OtaProgress";

describe("OtaProgress", () => {
  it("show/hide and update label and progress", () => {
    const ota = createOtaProgress();
    document.body.append(ota.root);
    ota.show();
    expect(ota.root.hidden).toBe(false);
    expect(ota.root.getAttribute("aria-busy")).toBe("true");
    ota.setLabel("Downloading…");
    expect(ota.root.textContent).toContain("Downloading");
    ota.setProgress(25, 100);
    const bar = ota.root.querySelector(".progress__bar") as HTMLElement;
    expect(bar.style.width).toBe("25%");
    expect(bar.getAttribute("aria-valuenow")).toBe("25");
    ota.setProgress(0, 0);
    expect(bar.style.width).toBe("0%");
    ota.setIndeterminate(true);
    expect(ota.root.querySelector(".progress--indeterminate")).not.toBeNull();
    expect(bar.getAttribute("aria-valuenow")).toBeNull();
    ota.setIndeterminate(false);
    ota.setTrackVisible(true);
    expect(ota.root.querySelector(".ota-progress__track")?.hidden).toBe(false);
    ota.setTrackVisible(false);
    expect(ota.root.querySelector(".ota-progress__track")?.hidden).toBe(true);
    ota.setSpinnerVisible(true);
    expect(ota.root.querySelector(".inline-spinner")?.hidden).toBe(false);
    ota.setSpinnerVisible(false);
    expect(ota.root.querySelector(".inline-spinner")?.hidden).toBe(true);
    ota.reset();
    ota.hide();
    expect(ota.root.hidden).toBe(true);
    expect(ota.root.getAttribute("aria-busy")).toBe("false");
  });
});
