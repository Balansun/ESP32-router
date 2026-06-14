import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { buildSelfTestPanel } from "./SelfTestPanel";

const apiMock = vi.hoisted(() => ({
  getHealth: vi.fn(),
  postHealthSelfTestRun: vi.fn(),
  postHealthSelfTestSkip: vi.fn(),
}));

const pollStop = vi.fn();
const pollMock = vi.hoisted(() => ({
  poll: vi.fn(() => ({ stop: pollStop })),
}));

vi.mock("../api/client", () => ({ api: apiMock }));
vi.mock("../state/store", async (importOriginal) => {
  const actual = await importOriginal<typeof import("../state/store")>();
  return { ...actual, poll: pollMock.poll };
});
vi.mock("./Toast", () => ({ toast: vi.fn() }));

describe("buildSelfTestPanel", () => {
  let signal: AbortSignal;

  beforeEach(() => {
    vi.clearAllMocks();
    signal = new AbortController().signal;
    apiMock.getHealth.mockResolvedValue({ ok: true, self_test: undefined });
    apiMock.postHealthSelfTestRun.mockResolvedValue({ ok: true, running: true });
    apiMock.postHealthSelfTestSkip.mockResolvedValue({ ok: true, skipped: true });
  });

  afterEach(() => {
    vi.restoreAllMocks();
  });

  it("renders pending state with run and skip actions", () => {
    const panel = buildSelfTestPanel({ signal });
    panel.setSelfTest({
      pending: true,
      running: false,
      skipped: false,
      results: { zc_ok: true, triac_ok: true, source_ok: true, zc_edges_per_sec: 100 },
    });
    const body = panel.el.querySelector(".alert-stack");
    expect(body?.textContent).toMatch(/self-test|commissioning/i);
    expect(panel.el.querySelectorAll("button").length).toBeGreaterThanOrEqual(2);
  });

  it("renders running state", () => {
    const panel = buildSelfTestPanel({ signal });
    panel.setSelfTest({ pending: false, running: true, skipped: false });
    expect(panel.el.querySelector(".alert--loading")).toBeTruthy();
  });

  it("renders success when all checks pass", () => {
    const panel = buildSelfTestPanel({ signal });
    panel.setSelfTest({
      pending: false,
      running: false,
      skipped: false,
      last_run_epoch: 1_700_000_000,
      results: { zc_ok: true, triac_ok: true, source_ok: true, zc_edges_per_sec: 50 },
    });
    expect(panel.el.querySelector(".alert--success")).toBeTruthy();
    expect(panel.el.textContent).toMatch(/50/);
  });

  it("hides last run when epoch is device uptime, not wall clock", () => {
    const panel = buildSelfTestPanel({ signal });
    panel.setSelfTest({
      pending: false,
      running: false,
      skipped: false,
      last_run_epoch: 529,
      results: { zc_ok: true, triac_ok: true, source_ok: true, zc_edges_per_sec: 50 },
    });
    expect(panel.el.textContent).not.toMatch(/1970/);
    expect(panel.el.textContent).not.toMatch(/Last test|Dernier test/i);
  });

  it("renders failure alerts for failed checks", () => {
    const panel = buildSelfTestPanel({ signal });
    panel.setSelfTest({
      pending: false,
      running: false,
      skipped: false,
      last_run_epoch: 1,
      results: { zc_ok: false, triac_ok: false, source_ok: false, zc_edges_per_sec: 0 },
    });
    expect(panel.el.querySelectorAll(".alert--danger, .alert--warn").length).toBe(3);
  });

  it("emphasizes lockout with critical severity and hides skip", () => {
    const panel = buildSelfTestPanel({ signal });
    panel.setSelfTest({
      pending: false,
      running: false,
      skipped: false,
      safety_lockout_active: true,
      severity: { zc: "critical", triac: "ok", source: "ok" },
      last_run_epoch: 1,
      results: { zc_ok: false, triac_ok: true, source_ok: true, zc_edges_per_sec: 0 },
    });
    expect(panel.el.querySelectorAll(".alert--danger").length).toBeGreaterThanOrEqual(2);
    expect(panel.el.textContent).toMatch(/routing disabled|safety/i);
    expect(panel.el.querySelector(".btn--primary")).toBeTruthy();
    expect(
      Array.from(panel.el.querySelectorAll("button")).some((b) =>
        /skip/i.test(b.textContent ?? ""),
      ),
    ).toBe(false);
  });

  it("refresh loads health and re-renders", async () => {
    apiMock.getHealth.mockResolvedValue({
      ok: true,
      self_test: {
        pending: false,
        running: false,
        skipped: true,
        results: { zc_ok: true, triac_ok: true, source_ok: true, zc_edges_per_sec: 10 },
      },
    });
    const panel = buildSelfTestPanel({ signal });
    await panel.refresh();
    expect(apiMock.getHealth).toHaveBeenCalled();
    expect(panel.el.querySelector(".alert--success")).toBeTruthy();
  });

  it("starts fast poll when refresh sees running test", async () => {
    apiMock.getHealth.mockResolvedValue({
      ok: true,
      self_test: { pending: false, running: true, skipped: false },
    });
    const panel = buildSelfTestPanel({ signal });
    await panel.refresh();
    expect(pollMock.poll).toHaveBeenCalled();
  });

  it("setSelfTest starts poll when running", () => {
    const panel = buildSelfTestPanel({ signal });
    panel.setSelfTest({ pending: false, running: true, skipped: false });
    expect(pollMock.poll).toHaveBeenCalled();
  });

  it("stop clears poll handle", () => {
    const panel = buildSelfTestPanel({ signal });
    panel.setSelfTest({ pending: false, running: true, skipped: false });
    panel.stop();
    expect(pollStop).toHaveBeenCalled();
  });

  it("run and skip buttons call API from pending state", async () => {
    const panel = buildSelfTestPanel({ signal });
    panel.setSelfTest({ pending: true, running: false, skipped: false });
    const runBtn = panel.el.querySelector(".btn--primary") as HTMLButtonElement;
    const skipBtn = panel.el.querySelector(".btn--ghost") as HTMLButtonElement;
    runBtn?.click();
    await vi.waitFor(() => expect(apiMock.postHealthSelfTestRun).toHaveBeenCalled());
    apiMock.getHealth.mockResolvedValue({
      ok: true,
      self_test: { pending: true, running: false, skipped: false },
    });
    skipBtn?.click();
    await vi.waitFor(() => expect(apiMock.postHealthSelfTestSkip).toHaveBeenCalled());
  });
});
