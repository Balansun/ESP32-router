import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { ApiError } from "../../api/client";
import type { RouterConfig } from "../../api/types";
import { deviceInfo } from "../../state/store";
import { mountSettingsLayout } from "./settingsLayout";
import {
  mockDeviceFromRow,
  profileMatrixVariants,
} from "../../../test/utils/profileMatrix";

const apiMock = vi.hoisted(() => ({
  getConfig: vi.fn(),
  getDevice: vi.fn(),
  getHealth: vi.fn(),
  getState: vi.fn(),
  getSystemPins: vi.fn(),
  getTime: vi.fn(),
  getMeasurements: vi.fn(),
  getTariffTempo: vi.fn(),
  patchConfig: vi.fn(),
}));

vi.mock("../../api/client", async (importOriginal) => {
  const actual = await importOriginal<typeof import("../../api/client")>();
  return { ...actual, api: apiMock };
});
vi.mock("../../components/Toast", () => ({ toast: vi.fn() }));
vi.mock("../../components/Dialog", () => ({
  openDialog: vi.fn(() => ({ close: () => {} })),
}));

function routerConfig(): RouterConfig {
  return {
    source: "JsyMk194",
    dhcp_on: true,
    router_name: "Balansun",
    install_country: "FR",
    vacation_enabled: false,
    vacation_end_epoch: 0,
    calib_u: 1000,
    calib_i: 1000,
    pwm_mode: "off",
    pwm_gpio: -1,
    pmqtt_topic: "",
    jsy_mk333_serial_baud: 9600,
    triac_override_max_temp_c: 70,
    mqtt_repeat_sec: 0,
  } as RouterConfig;
}

describe("PWM settings save via API", () => {
  const outlet = document.createElement("div");
  let configStore: RouterConfig;

  beforeEach(() => {
    vi.clearAllMocks();
    configStore = routerConfig();
    deviceInfo.set(undefined);
    outlet.replaceChildren();
    document.body.append(outlet);

    apiMock.getConfig.mockImplementation(async () => ({
      schema_version: 5,
      config: { ...configStore },
    }));
    apiMock.patchConfig.mockImplementation(async (patch: Partial<RouterConfig>) => {
      Object.assign(configStore, patch);
    });

    const matrixRow = profileMatrixVariants.find(
      (r) => r.product_profile === "jsy_mk194_router",
    )!;
    const row = mockDeviceFromRow(matrixRow);

    apiMock.getDevice.mockResolvedValue(row);
    apiMock.getHealth.mockResolvedValue({ ok: true, hardware: { items: [] } });
    apiMock.getState.mockRejectedValue(new ApiError("not_ready", 503, {}));
    apiMock.getSystemPins.mockResolvedValue({
      board_pin_profile: "wroom32",
      pin_triac_dim: 22,
      pin_zero_cross: 23,
      pin_uart_rx: 26,
      pin_uart_tx: 27,
      pin_temp: -1,
      pin_analog0: 35,
      pin_analog1: 32,
      pin_analog2: 33,
    });
    apiMock.getTime.mockResolvedValue({
      tz: "Europe/Paris",
      ntp1: "",
      ntp2: "",
      date_valid: true,
      now: "2026-06-05T12:00:00",
    });
    apiMock.getMeasurements.mockResolvedValue(null);
    apiMock.getTariffTempo.mockResolvedValue({ today: null, tomorrow: null });
    deviceInfo.set(row);
  });

  afterEach(() => {
    outlet.replaceChildren();
  });

  it("patchConfig persists PWM selects and getConfig returns updated values", async () => {
    const ctrl = new AbortController();
    const cleanup = await mountSettingsLayout({
      path: "/settings/advanced",
      outlet,
      signal: ctrl.signal,
    });

    const pwmMode = outlet.querySelector("#pwm_mode") as HTMLSelectElement;
    const pwmGpio = outlet.querySelector("#pwm_gpio") as HTMLSelectElement;
    expect(pwmMode).toBeTruthy();
    expect(pwmGpio).toBeTruthy();

    pwmMode.value = "independent";
    pwmMode.dispatchEvent(new Event("change", { bubbles: true }));
    pwmGpio.value = "4";
    pwmGpio.dispatchEvent(new Event("change", { bubbles: true }));

    const saveBtn = outlet.querySelector<HTMLButtonElement>("button.btn--primary");
    expect(saveBtn).toBeTruthy();
    expect(saveBtn!.disabled).toBe(false);
    saveBtn!.click();
    await vi.waitFor(() => expect(apiMock.patchConfig).toHaveBeenCalled());

    expect(apiMock.patchConfig).toHaveBeenCalledWith(
      expect.objectContaining({ pwm_mode: "independent", pwm_gpio: 4 }),
      expect.anything(),
    );

    const env = await apiMock.getConfig();
    expect(env.config.pwm_mode).toBe("independent");
    expect(env.config.pwm_gpio).toBe(4);

    cleanup();
    ctrl.abort();
  });
});
