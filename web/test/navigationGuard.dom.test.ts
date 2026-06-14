import { beforeEach, describe, expect, it, vi } from "vitest";

vi.mock("../src/components/Dialog", () => ({
  openDialog: vi.fn((opts: { actions: Array<{ onClick: () => void }> }) => {
    opts.actions[1]?.onClick();
    return { close: () => {} };
  }),
}));
import {
  confirmDiscardChanges,
  hasUnsavedChanges,
  setUnsavedGuard,
} from "../src/navigationGuard";

describe("navigationGuard", () => {
  beforeEach(() => {
    setUnsavedGuard(null);
  });

  it("hasUnsavedChanges reflects guard", () => {
    expect(hasUnsavedChanges()).toBe(false);
    setUnsavedGuard(() => true);
    expect(hasUnsavedChanges()).toBe(true);
  });

  it("confirmDiscardChanges resolves true when clean", async () => {
    expect(await confirmDiscardChanges()).toBe(true);
  });

  it("confirmDiscardChanges opens dialog when dirty", async () => {
    setUnsavedGuard(() => true);
    expect(await confirmDiscardChanges()).toBe(true);
  });

  it("confirmDiscardChanges resolves false when user cancels", async () => {
    const { openDialog } = await import("../src/components/Dialog");
    vi.mocked(openDialog).mockImplementationOnce((opts) => {
      opts.actions[0]?.onClick();
      return { close: () => {} };
    });
    setUnsavedGuard(() => true);
    expect(await confirmDiscardChanges()).toBe(false);
  });
});
