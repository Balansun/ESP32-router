import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { ApiError } from "../../api/client";
import type { RouterConfig } from "../../api/types";
import { getStrings } from "../../i18n";
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

function baseConfig(): RouterConfig {
  return {
    source: "JsyMk194",
    dhcp_on: true,
    router_name: "Balansun",
    install_country: "FR",
    vacation_enabled: false,
    vacation_end_epoch: 0,
    calib_u: 1000,
    calib_i: 1000,
    pmqtt_topic: "",
    jsy_mk333_serial_baud: 9600,
    triac_override_max_temp_c: 70,
    mqtt_repeat_sec: 0,
  } as RouterConfig;
}

function setupApiForRow(row: ReturnType<typeof mockDeviceFromRow>) {
  apiMock.getConfig.mockResolvedValue({ schema_version: 5, config: baseConfig() });
  apiMock.getDevice.mockResolvedValue(row);
  apiMock.getHealth.mockResolvedValue({ ok: true, hardware: { items: [] } });
  apiMock.getState.mockRejectedValue(new ApiError("not_ready", 503, {}));
  apiMock.getSystemPins.mockResolvedValue({
    board_pin_profile: row.capabilities?.product_profile?.includes("32u")
      ? "esp32_32u"
      : "wroom32",
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
  apiMock.patchConfig.mockResolvedValue(undefined);
}

describe("settingsLayout profile matrix gates", () => {
  const outlet = document.createElement("div");

  beforeEach(() => {
    vi.clearAllMocks();
    deviceInfo.set(undefined);
    outlet.replaceChildren();
    document.body.append(outlet);
  });

  afterEach(() => {
    outlet.replaceChildren();
  });

  it.each(profileMatrixVariants.map((row) => [row.product_profile, row] as const))(
    "%s advanced PWM gates",
    async (_profile, row) => {
      const device = mockDeviceFromRow(row);
      setupApiForRow(device);
      deviceInfo.set(device);

      const ac = new AbortController();
      const cleanup = await mountSettingsLayout({
        path: "/settings/advanced",
        outlet,
        signal: ac.signal,
      });

      const pwmMode = outlet.querySelector("#pwm_mode") as HTMLSelectElement | null;
      expect(pwmMode?.tagName).toBe("SELECT");
      const followOpt = outlet.querySelector<HTMLOptionElement>(
        '#pwm_mode option[value="follow_triac"]',
      );
      expect(followOpt?.hidden).toBe(!row.ui.show_follow_triac_pwm_option);

      const visibleTitles = Array.from(outlet.querySelectorAll(".settings-section .card"))
        .filter((c) => !(c as HTMLElement).hidden)
        .map((c) => c.querySelector(".section__title")?.textContent?.trim());
      const T = getStrings();
      if (row.ui.show_advanced_meter_sources) {
        expect(visibleTitles).toContain(T.settings.sectionAdvancedSources);
      } else {
        expect(visibleTitles).not.toContain(T.settings.sectionAdvancedSources);
      }

      cleanup();
      ac.abort();
    },
  );

  it.each(profileMatrixVariants.map((row) => [row.product_profile, row] as const))(
    "%s metering gates",
    async (_profile, row) => {
      const device = mockDeviceFromRow(row);
      setupApiForRow(device);
      deviceInfo.set(device);

      const T = getStrings();
      const ac = new AbortController();
      const cleanup = await mountSettingsLayout({
        path: "/settings/metering",
        outlet,
        signal: ac.signal,
      });

      const visibleMetering = Array.from(outlet.querySelectorAll(".settings-section .card"))
        .filter((c) => !(c as HTMLElement).hidden);
      const meteringText = visibleMetering.map((el) => el.textContent).join(" ");
      if (row.ui.show_measurement_source) {
        expect(meteringText).toContain(T.settings.sectionMeasurement);
      } else {
        expect(meteringText).not.toContain(T.settings.openSourceWizard);
        expect(meteringText).not.toContain(T.settings.sectionMeasurement);
      }

      cleanup();
      ac.abort();
    },
  );
});
