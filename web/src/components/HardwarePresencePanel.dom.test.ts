import { describe, expect, it, vi, beforeEach } from "vitest";
import type { HardwarePresenceItem } from "../api/types";
import { getStrings } from "../i18n";
import { buildHardwarePresencePanel } from "./HardwarePresencePanel";

const apiMock = vi.hoisted(() => ({
  postHealthHardwareRecheck: vi.fn(),
}));

const toastMock = vi.hoisted(() => vi.fn());

vi.mock("../api/client", () => ({ api: apiMock }));
vi.mock("./Toast", () => ({ toast: toastMock }));

describe("buildHardwarePresencePanel", () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  it("renders alerts and supports recheck actions", async () => {
    const T = getStrings();
    const signal = new AbortController().signal;

    const warningBase: HardwarePresenceItem = {
      id: "jsy_mk194",
      role: "jsy_mk194",
      state: "missing",
      gpio_rx: 1,
      gpio_tx: 2,
      detail: "missing detail",
    };

    const okBase: HardwarePresenceItem = {
      id: "zero_cross",
      role: "zero_cross",
      state: "ok",
      gpio: 23,
      detail: "ok detail",
    };

    const mutedBase: HardwarePresenceItem = {
      id: "temp_ds18b20",
      role: "temp_ds18b20",
      state: "not_applicable",
      gpio: 13,
      detail: "muted detail",
    };

    const itemsById: Record<string, HardwarePresenceItem> = {
      [warningBase.id]: warningBase,
      [okBase.id]: okBase,
      [mutedBase.id]: mutedBase,
    };

    apiMock.postHealthHardwareRecheck.mockImplementation(async (id: string) => {
      const current = itemsById[id];
      const next: HardwarePresenceItem = {
        ...current,
        state: "ok",
        detail: "rechecked ok",
      };
      itemsById[id] = next;
      return { item: next };
    });

    const onItemsChange = vi.fn();
    const handle = buildHardwarePresencePanel({ signal, onItemsChange });

    handle.setItems([warningBase, okBase, mutedBase]);

    const warnAlert = handle.el.querySelector(".hw-presence__warnings .alert") as HTMLElement | null;
    const mutedAlert = handle.el.querySelector(".hw-presence__muted .alert") as HTMLElement | null;
    expect(warnAlert).toBeTruthy();
    expect(mutedAlert).toBeTruthy();

    const recheckAllBtn = handle.el.querySelector(".hw-presence__head button") as HTMLButtonElement | null;
    expect(recheckAllBtn).toBeTruthy();
    recheckAllBtn!.click();

    await vi.waitFor(() => expect(apiMock.postHealthHardwareRecheck).toHaveBeenCalledTimes(1));
    expect(toastMock).toHaveBeenCalledWith(T.diag.hardwareRecheckDone, "success");

    // Reset to "missing" again so we can cover recheckOne.
    const warningMissingAgain: HardwarePresenceItem = { ...warningBase, state: "missing" };
    handle.setItems([warningMissingAgain, okBase, mutedBase]);
    apiMock.postHealthHardwareRecheck.mockClear();
    toastMock.mockClear();
    onItemsChange.mockClear();

    const recheckOneBtn = handle.el.querySelector(".hw-presence__warnings .alert button") as HTMLButtonElement | null;
    expect(recheckOneBtn).toBeTruthy();
    recheckOneBtn!.click();

    await vi.waitFor(() => expect(apiMock.postHealthHardwareRecheck).toHaveBeenCalledTimes(1));
    expect(toastMock).toHaveBeenCalledWith(T.diag.hardwareRecheckDone, "success");
    expect(onItemsChange).toHaveBeenCalled();
  });
});

