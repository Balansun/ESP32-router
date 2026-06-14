import { describe, expect, it, vi, beforeEach, afterEach } from "vitest";
import type { RouterConfig } from "../../api/types";
import { getStrings } from "../../i18n";
import { deviceInfo, localePref } from "../../state/store";
import { hardwarePinDefaultsForBoardEnv } from "./hardwarePinDefaults";
import { buildHardwarePinsCard } from "./hardwarePinsSettings";

const apiMock = vi.hoisted(() => ({
  putSystemPins: vi.fn(),
  resetPins: vi.fn(),
  getSystemPins: vi.fn(),
  getHealth: vi.fn().mockResolvedValue(null),
}));

const toastMock = vi.hoisted(() => vi.fn());

const openDialogMock = vi.hoisted(() => vi.fn(() => ({ close: () => {} })));

vi.mock("../../api/client", () => ({ api: apiMock }));
vi.mock("../../components/Toast", () => ({ toast: toastMock }));
vi.mock("../../components/Dialog", () => ({ openDialog: openDialogMock }));

describe("buildHardwarePinsCard", () => {
  beforeEach(() => {
    vi.useFakeTimers();
    vi.clearAllMocks();

    apiMock.putSystemPins.mockResolvedValue({ ok: true, reboot_required: false });
    apiMock.resetPins.mockResolvedValue({ ok: true });
    apiMock.getHealth.mockResolvedValue(null);
    deviceInfo.set({
      router_name: "Test",
      firmware_version: "0.0.0",
      probe_house_name: "",
      probe_second_name: "",
      temperature_label: "",
      capabilities: {
        product_profile: "full_router",
        firmware_capabilities: { triac: true, surplus_regulation: true },
      },
    });

    apiMock.getSystemPins.mockResolvedValue({
      board_pin_profile: "wroom32",
      pin_default_triac_dim: 22,
      pin_default_zero_cross: 23,
      pin_default_uart_rx: 26,
      pin_default_uart_tx: 27,
      pin_default_temp: 13,
      pin_default_analog0: 35,
      pin_default_analog1: 32,
      pin_default_analog2: 33,

      pin_triac_dim: 22,
      pin_zero_cross: 23,
      pin_uart_rx: 26,
      pin_uart_tx: 27,
      // Drive the temp disabled branch (v === -1) in `tempRow.ref.write()`.
      pin_temp: -1,
      pin_analog0: 35,
      pin_analog1: 32,
      pin_analog2: 33,
    });
  });

  afterEach(() => {
    vi.clearAllTimers();
    vi.useRealTimers();
  });

  it("builds, toggles effective source, saves, and resets", async () => {
    const T = getStrings();
    const pinDefaults = hardwarePinDefaultsForBoardEnv("wroom32");

    const cfg = {
      source: "JsyMk194",
      pin_map_reboot_pending: false,
      pin_map_fallback_active: false,

      pin_triac_dim: pinDefaults.pin_triac_dim,
      pin_zero_cross: pinDefaults.pin_zero_cross,
      pin_uart_rx: pinDefaults.pin_uart_rx,
      pin_uart_tx: pinDefaults.pin_uart_tx,
      pin_analog0: pinDefaults.pin_analog0,
      pin_analog1: pinDefaults.pin_analog1,
      pin_analog2: pinDefaults.pin_analog2,
      pin_temp: pinDefaults.pin_temp,

      board_pin_profile: "wroom32",
    } as RouterConfig;

    const ctrl = new AbortController();
    const onSaved = vi.fn();

    const handle = buildHardwarePinsCard(
      cfg,
      pinDefaults,
      "wroom32",
      undefined,
      ctrl.signal,
      { pending: "pending", saving: "saving", saved: "saved", error: "error" },
      () => {},
      onSaved,
    );

    expect(handle.card.querySelector('input.field__input[type="number"]')).toBeTruthy();

    // Cover updateConflict + summary refresh for both meter-source branches.
    handle.setEffectiveSource("uxi");
    handle.setEffectiveSource("JsyMk194");

    // Cover autosave validate/collect/persist.
    handle.saver.markDirty();
    await handle.saver.flush();
    await vi.runOnlyPendingTimersAsync();
    expect(apiMock.putSystemPins).toHaveBeenCalled();
    expect(onSaved).toHaveBeenCalled();

    // Cover reset dialog action path.
    const resetBtn = handle.card.querySelector("button.btn--ghost") as HTMLButtonElement;
    expect(resetBtn).toBeTruthy();
    resetBtn.click();

    const dialogOpts = openDialogMock.mock.calls.at(-1)?.[0];
    expect(dialogOpts?.actions?.length).toBeGreaterThanOrEqual(2);

    await dialogOpts.actions[1].onClick();
    expect(apiMock.resetPins).toHaveBeenCalled();
    expect(apiMock.getSystemPins).toHaveBeenCalled();
    expect(toastMock).toHaveBeenCalledWith(T.settings.hardwareResetOk, "success");

    // Cover temp mode change handler (dispatch `change` on the select).
    const tempModeSelect = handle.card.querySelector("#pin_temp_mode") as HTMLSelectElement;
    const tempGpioInput = handle.card.querySelector("#pin_temp_gpio") as HTMLInputElement;
    expect(tempModeSelect.value).toBe("disabled");
    tempModeSelect.value = "enabled";
    tempModeSelect.dispatchEvent(new Event("change", { bubbles: true }));
    expect(tempGpioInput.hidden).toBe(false);

    // Cover `tempRow.ref.write(v !== -1)` path in the reset handler.
    apiMock.getSystemPins.mockResolvedValueOnce({
      board_pin_profile: "wroom32",
      pin_default_triac_dim: 22,
      pin_default_zero_cross: 23,
      pin_default_uart_rx: 26,
      pin_default_uart_tx: 27,
      pin_default_temp: 13,
      pin_default_analog0: 35,
      pin_default_analog1: 32,
      pin_default_analog2: 33,

      pin_triac_dim: 22,
      pin_zero_cross: 23,
      pin_uart_rx: 26,
      pin_uart_tx: 27,
      pin_temp: 13,
      pin_analog0: 35,
      pin_analog1: 32,
      pin_analog2: 33,
    });

    resetBtn.click();
    const dialogOptsSuccess2 = openDialogMock.mock.calls.at(-1)?.[0];
    await dialogOptsSuccess2.actions[1].onClick();
    expect(toastMock).toHaveBeenCalledWith(T.settings.hardwareResetOk, "success");

    // Cover duplicate conflict validation path (dup !== null -> error toast + no persist call).
    const coreInputs = Array.from(
      handle.card.querySelectorAll<HTMLInputElement>("input.field__input[type='number']"),
    ).filter((el) => el.id !== "pin_temp_gpio");
    expect(coreInputs.length).toBeGreaterThanOrEqual(2);
    const dupGpio = 7;
    coreInputs[0].value = String(dupGpio);
    coreInputs[1].value = String(dupGpio);
    toastMock.mockClear();
    apiMock.putSystemPins.mockClear();
    coreInputs[0].dispatchEvent(new Event("input", { bubbles: true }));

    handle.saver.markDirty();
    await handle.saver.flush();
    await vi.runOnlyPendingTimersAsync();

    expect(apiMock.putSystemPins).not.toHaveBeenCalled();
    expect(
      toastMock.mock.calls.some(
        ([msg, kind]) => kind === "error" && String(msg).includes(String(dupGpio)),
      ),
    ).toBe(true);

    // Cover reset error path (catch branch -> T.saveError toast).
    toastMock.mockClear();
    apiMock.resetPins.mockRejectedValueOnce(new Error("reset failed"));
    resetBtn.click();
    const dialogOpts2 = openDialogMock.mock.calls.at(-1)?.[0];
    await dialogOpts2.actions[1].onClick();
    expect(toastMock).toHaveBeenCalledWith(T.saveError, "error");

    // Saved on successful persist + successful reset only.
    expect(onSaved).toHaveBeenCalledTimes(3);
    ctrl.abort();
  });

  it("covers fr lang, cfg fallbacks, and reboot_required=true persist", async () => {
    localePref.set("fr");

    const pinDefaults = hardwarePinDefaultsForBoardEnv("wroom32");

    // Omit `pin_triac_dim` and `pin_temp` so `readCorePin()` / `readTempPin()` use defaults.
    const cfg = {
      source: "JsyMk194",
      pin_map_reboot_pending: false,
      pin_map_fallback_active: false,

      pin_zero_cross: pinDefaults.pin_zero_cross,
      pin_uart_rx: pinDefaults.pin_uart_rx,
      pin_uart_tx: pinDefaults.pin_uart_tx,
      pin_analog0: pinDefaults.pin_analog0,
      pin_analog1: pinDefaults.pin_analog1,
      pin_analog2: pinDefaults.pin_analog2,

      board_pin_profile: "wroom32",
    } as RouterConfig;

    apiMock.putSystemPins.mockResolvedValueOnce({ ok: true, reboot_required: true });

    const ctrl = new AbortController();
    const onSaved = vi.fn();
    const handle = buildHardwarePinsCard(
      cfg,
      pinDefaults,
      "wroom32",
      undefined,
      ctrl.signal,
      { pending: "pending", saving: "saving", saved: "saved", error: "error" },
      () => {},
      onSaved,
    );

    handle.saver.markDirty();
    await handle.saver.flush();
    await vi.runOnlyPendingTimersAsync();

    expect(apiMock.putSystemPins).toHaveBeenCalled();
    expect(onSaved).toHaveBeenCalled();
    ctrl.abort();
  });

  it("covers temp disabled init, reset fallbacks, and NaN reads", async () => {
    const pinDefaults = hardwarePinDefaultsForBoardEnv("wroom32");

    const cfg = {
      source: "JsyMk194",
      pin_map_reboot_pending: false,
      pin_map_fallback_active: false,

      pin_triac_dim: pinDefaults.pin_triac_dim,
      pin_zero_cross: pinDefaults.pin_zero_cross,
      pin_uart_rx: pinDefaults.pin_uart_rx,
      pin_uart_tx: pinDefaults.pin_uart_tx,
      pin_analog0: pinDefaults.pin_analog0,
      pin_analog1: pinDefaults.pin_analog1,
      pin_analog2: pinDefaults.pin_analog2,
      // Drive `disabled === true` in `buildTempPinRow()`.
      pin_temp: -1,

      board_pin_profile: "wroom32",
    } as RouterConfig;

    const ctrl = new AbortController();
    const onSaved = vi.fn();
    const handle = buildHardwarePinsCard(
      cfg,
      pinDefaults,
      "wroom32",
      undefined,
      ctrl.signal,
      { pending: "pending", saving: "saving", saved: "saved", error: "error" },
      () => {},
      onSaved,
    );

    const resetBtn = handle.card.querySelector("button.btn--ghost") as HTMLButtonElement;
    expect(resetBtn).toBeTruthy();

    // Trigger reset handler fallback branches:
    // - missing `board_pin_profile` -> uses `cfg.board_pin_profile`
    // - missing pin_* value -> uses defaults via `readCorePinFromMap()`
    // - missing `pin_temp` -> uses defaults via `readTempPinFromMap()`
    apiMock.getSystemPins.mockResolvedValueOnce({
      pin_default_triac_dim: 22,
      pin_default_zero_cross: 23,
      pin_default_uart_rx: 26,
      pin_default_uart_tx: 27,
      pin_default_temp: 13,
      pin_default_analog0: 35,
      pin_default_analog1: 32,
      pin_default_analog2: 33,

      pin_triac_dim: 22,
      pin_zero_cross: 23,
      // Intentionally omit `pin_uart_tx`.
      pin_uart_rx: 26,
      // Intentionally omit `pin_temp`.
      pin_analog0: 35,
      pin_analog1: 32,
      pin_analog2: 33,
    } as any);

    resetBtn.click();
    const dialogOpts = openDialogMock.mock.calls.at(-1)?.[0];
    await dialogOpts.actions[1].onClick();

    expect(toastMock).toHaveBeenCalledWith(getStrings().settings.hardwareResetOk, "success");

    // Cover NaN branches in `readVisible()`:
    // - core pin Number('abc') => NaN
    // - temp gpio parseInt('abc') => NaN
    const coreInputs = Array.from(
      handle.card.querySelectorAll<HTMLInputElement>("input.field__input[type='number']"),
    ).filter((el) => el.id !== "pin_temp_gpio");

    coreInputs[0].value = "abc";
    coreInputs[0].dispatchEvent(new Event("input", { bubbles: true }));

    const tempModeSelect = handle.card.querySelector("#pin_temp_mode") as HTMLSelectElement;
    const tempGpioInput = handle.card.querySelector("#pin_temp_gpio") as HTMLInputElement;
    tempModeSelect.value = "enabled";
    tempGpioInput.hidden = false;
    tempGpioInput.value = "abc";
    tempGpioInput.dispatchEvent(new Event("input", { bubbles: true }));

    expect(onSaved).toHaveBeenCalled();
    ctrl.abort();
  });
});

