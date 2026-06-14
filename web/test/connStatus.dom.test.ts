import { beforeEach, describe, expect, it, vi } from "vitest";
import {
  reportPollFailure,
  reportPollSuccess,
  resetConnStatus,
  startConnHealthMonitor,
  stopConnHealthMonitor,
} from "../src/state/connStatus";
import { conn } from "../src/state/store";

vi.mock("../src/paths", () => ({
  stripBase: vi.fn((p: string) => p || "/"),
}));

import { stripBase } from "../src/paths";

describe("connStatus", () => {
  beforeEach(() => {
    vi.mocked(stripBase).mockImplementation((p) => p || "/");
    resetConnStatus("loading");
    stopConnHealthMonitor();
  });

  it("skips monitor on wifi setup path", () => {
    vi.mocked(stripBase).mockReturnValue("/wifi");
    const stop = startConnHealthMonitor();
    expect(typeof stop).toBe("function");
  });

  it("resetConnStatus and poll report helpers", () => {
    resetConnStatus("ok");
    expect(conn.get()).toBe("ok");
    reportPollSuccess();
    reportPollFailure();
  });
});
