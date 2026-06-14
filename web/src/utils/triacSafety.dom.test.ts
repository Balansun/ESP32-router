import { describe, expect, it, vi } from "vitest";

vi.mock("../components/Dialog", () => ({
  openDialog: vi.fn((opts: { actions: Array<{ onClick: () => void }> }) => {
    opts.actions[1]?.onClick();
    return { close: () => {} };
  }),
}));

import { confirmLegionellaTriacFull } from "./triacSafety";

describe("confirmLegionellaTriacFull", () => {
  it("resolves true when user confirms", async () => {
    await expect(confirmLegionellaTriacFull()).resolves.toBe(true);
  });

  it("resolves false when user cancels", async () => {
    const { openDialog } = await import("../components/Dialog");
    vi.mocked(openDialog).mockImplementationOnce((opts) => {
      opts.actions[0]?.onClick();
      return { close: () => {} };
    });
    await expect(confirmLegionellaTriacFull()).resolves.toBe(false);
  });
});
