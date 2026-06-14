import { beforeEach, describe, expect, it, vi } from "vitest";
import { confirmRestoreBackupFromFile } from "../src/utils/backupApply";
import { buildBackup } from "../src/utils/backupFormat";
import { ensureRouterConfigPutPayload } from "../src/api/configPut";
import type { ActionsConfigEnvelope, RouterConfig } from "../src/api/types";

const apiMock = vi.hoisted(() => ({
  putSystemBackup: vi.fn(),
  getDevice: vi.fn(),
}));

vi.mock("../src/api/client", () => ({ api: apiMock }));
vi.mock("../src/components/Toast", () => ({ toast: vi.fn() }));
vi.mock("../src/components/Dialog", () => ({
  openDialog: vi.fn((opts: {
    actions: Array<{ onClick: () => void | Promise<void> }>;
  }) => {
    void Promise.resolve(opts.actions[1]?.onClick());
    return { close: () => {} };
  }),
}));

function sampleBackup() {
  const config = ensureRouterConfigPutPayload({
    source: "JsyMk194",
    router_name: "R",
  } as RouterConfig);
  const actions: ActionsConfigEnvelope = {
    schema_version: 4,
    nb_actions: 1,
    actions: [
      {
        index: 0,
        title: "T",
        kind: "triac",
        repeat_sec: 0,
        tempo_sec: 0,
        regulation_mode: 1,
        periods: [],
      },
    ],
  };
  return buildBackup(
    config,
    actions,
    { tz: "UTC", ntp1: "a", ntp2: "b" },
    { ssid: "net", password: "" },
  );
}

describe("backupApply DOM", () => {
  beforeEach(() => {
    vi.clearAllMocks();
    apiMock.putSystemBackup.mockResolvedValue(undefined);
    apiMock.getDevice.mockResolvedValue({
      router_name: "R",
      firmware_version: "1",
      probe_house_name: "H",
      probe_second_name: "S",
      temperature_label: "T",
    });
  });

  it("confirmRestoreBackupFromFile applies valid backup via dialog", async () => {
    const file = new File([JSON.stringify(sampleBackup())], "b.json");
    await confirmRestoreBackupFromFile(file);
    expect(apiMock.putSystemBackup).toHaveBeenCalled();
  });

  it("confirmRestoreBackupFromFile cancel does not apply", async () => {
    const { openDialog } = await import("../src/components/Dialog");
    vi.mocked(openDialog).mockImplementationOnce((opts) => {
      opts.actions[0]?.onClick();
      return { close: () => {} };
    });
    const file = new File([JSON.stringify(sampleBackup())], "b.json");
    await confirmRestoreBackupFromFile(file);
    expect(apiMock.putSystemBackup).not.toHaveBeenCalled();
  });

  it("confirmRestoreBackupFromFile uses unknown fallback for parse errors", async () => {
    const mod = await import("../src/utils/backupFormat");
    vi.spyOn(mod, "parseBackupJson").mockReturnValue({
      ok: false,
      errorKey: "not_in_catalog",
    });
    const file = new File(["{}"], "b.json");
    await confirmRestoreBackupFromFile(file);
    expect(apiMock.putSystemBackup).not.toHaveBeenCalled();
  });
});
