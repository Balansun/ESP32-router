import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { ApiError } from "../../api/client";
import type { DeviceInfo, RouterConfig } from "../../api/types";
import { getStrings } from "../../i18n";
import { deviceInfo } from "../../state/store";
import { mountSettingsLayout } from "./settingsLayout";

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

function jsySingleMeterDevice(): DeviceInfo {
  return {
    device_uid: "f81d5d470968",
    source: "JsyMk194",
    source_data: "JsyMk194",
    router_name: "Balansun",
    firmware_version: "0.1.0",
    probe_house_name: "House metering",
    probe_second_name: "Second channel",
    ext_peer_ip: "0.0.0.0",
    ext_peer_port: 80,
    ext_peer_path: "/api/v1/measurements",
    temperature_label: "temperature_c",
    chip: { model: "ESP32-D0WD-V3", revision: 3, cores: 2, flash_mb: 4 },
    capabilities: {
      product_profile: "jsy_mk194_router",
      firmware_capabilities: {
        surplus_regulation: true,
        meter_pack: "jsy_mk194",
        meters: ["JsyMk194"],
      },
    },
  };
}

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

describe("settingsLayout capability gates", () => {
  const outlet = document.createElement("div");

  beforeEach(() => {
    vi.clearAllMocks();
    deviceInfo.set(undefined);
    outlet.replaceChildren();
    document.body.append(outlet);

    apiMock.getConfig.mockResolvedValue({ schema_version: 5, config: baseConfig() });
    apiMock.getDevice.mockResolvedValue(jsySingleMeterDevice());
    apiMock.getHealth.mockResolvedValue({
      ok: true,
      hardware: { items: [] },
    });
    apiMock.getState.mockRejectedValue(new ApiError("not_ready", 503, {}));
    apiMock.getSystemPins.mockResolvedValue({
      board_pin_profile: "esp32_32u",
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
  });

  afterEach(() => {
    outlet.replaceChildren();
  });

  it("hides source wizard on metering tab for single-meter device", async () => {
    const T = getStrings();
    const cleanup = await mountSettingsLayout({
      path: "/settings/metering",
      outlet,
      signal: new AbortController().signal,
    });

    const visibleMetering = Array.from(outlet.querySelectorAll(".settings-section .card, .settings-section [class*='measurement']"))
      .filter((el) => !(el as HTMLElement).hidden);
    const meteringText = visibleMetering.map((el) => el.textContent).join(" ");
    expect(meteringText).not.toContain(T.settings.openSourceWizard);
    expect(meteringText).not.toContain(T.settings.sectionMeasurement);
    cleanup();
  });

  it("hides advanced meter sources card on advanced tab for single-meter device", async () => {
    const T = getStrings();
    const cleanup = await mountSettingsLayout({
      path: "/settings/advanced",
      outlet,
      signal: new AbortController().signal,
    });

    const visibleTitles = Array.from(outlet.querySelectorAll(".settings-section .card"))
      .filter((c) => !(c as HTMLElement).hidden)
      .map((c) => c.querySelector(".section__title")?.textContent?.trim());
    expect(visibleTitles).not.toContain(T.settings.sectionAdvancedSources);
    expect(visibleTitles).toContain(T.settings.sectionAdvancedOutput);
    cleanup();
  });

  it("uses select controls for pwm_mode and pwm_gpio on advanced tab", async () => {
    const cleanup = await mountSettingsLayout({
      path: "/settings/advanced",
      outlet,
      signal: new AbortController().signal,
    });

    const pwmMode = outlet.querySelector("#pwm_mode");
    const pwmGpio = outlet.querySelector("#pwm_gpio");
    expect(pwmMode?.tagName).toBe("SELECT");
    expect(pwmGpio?.tagName).toBe("SELECT");
    expect(pwmMode?.querySelectorAll("option")).toHaveLength(3);
    expect(pwmGpio?.querySelectorAll("option").length).toBeGreaterThan(1);
    cleanup();
  });

  it("hides follow_triac pwm mode when surplus regulation is absent", async () => {
    apiMock.getDevice.mockResolvedValue({
      ...jsySingleMeterDevice(),
      capabilities: {
        product_profile: "jsy_mk194_meter",
        firmware_capabilities: {
          surplus_regulation: false,
          meter_pack: "jsy_mk194",
          meters: ["JsyMk194"],
        },
      },
    });

    const cleanup = await mountSettingsLayout({
      path: "/settings/advanced",
      outlet,
      signal: new AbortController().signal,
    });

    const followOpt = outlet.querySelector<HTMLOptionElement>(
      '#pwm_mode option[value="follow_triac"]',
    );
    expect(followOpt?.hidden).toBe(true);
    expect((outlet.querySelector("#pwm_mode") as HTMLSelectElement).value).not.toBe("follow_triac");
    cleanup();
  });
});
