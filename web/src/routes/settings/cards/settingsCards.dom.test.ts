import { beforeEach, describe, expect, it, vi } from "vitest";
import { getStrings } from "../../../i18n";
import type { RouterConfig } from "../../../api/types";
import { deviceInfo } from "../../../state/store";
import { textRow } from "../formRows";
import { settingsSection } from "./section";
import { buildNetworkSettingsCard, buildMqttSettingsCard } from "./networkCard";
import { buildMeasurementSettingsCard } from "./measurementCard";
import { buildAdvancedSection } from "./advancedCard";

const apiMock = vi.hoisted(() => ({
  saveNow: vi.fn(),
  resetHistory: vi.fn(),
  factoryReset: vi.fn(),
}));

vi.mock("../../../api/client", () => ({ api: apiMock }));
vi.mock("../../../components/Toast", () => ({ toast: vi.fn() }));
vi.mock("../../../components/Dialog", () => ({
  openDialog: vi.fn(() => ({ close: () => {} })),
}));
vi.mock("../../../components/SourceSetupWizard", () => ({
  openSourceSetupWizard: vi.fn(),
}));

describe("settingsSection", () => {
  it("wraps rows in a card section with title", () => {
    const row = document.createElement("p");
    row.textContent = "row";
    const section = settingsSection("Title", row);
    expect(section.className).toBe("card");
    expect(section.querySelector(".section__title")?.textContent).toBe("Title");
    expect(section.contains(row)).toBe(true);
  });
});

describe("network settings cards", () => {
  const T = getStrings();

  it("buildNetworkSettingsCard includes wifi link and dhcp row", () => {
    const dhcp = document.createElement("div");
    const card = buildNetworkSettingsCard(
      T,
      document.createElement("div"),
      dhcp,
      document.createElement("div"),
      document.createElement("div"),
      document.createElement("div"),
      document.createElement("div"),
    );
    const wifiHref = card.querySelector('a[data-route="true"]')?.getAttribute("href");
    expect(wifiHref === "/wifi" || wifiHref === "/wifi/station").toBe(true);
    expect(card.contains(dhcp)).toBe(true);
  });

  it("buildMqttSettingsCard nests mqtt fields", () => {
    const device = document.createElement("div");
    device.id = "mqtt-device";
    const card = buildMqttSettingsCard(
      T,
      document.createElement("div"),
      document.createElement("div"),
      document.createElement("div"),
      document.createElement("div"),
      document.createElement("div"),
      document.createElement("div"),
      document.createElement("div"),
      device,
    );
    expect(card.contains(device)).toBe(true);
  });
});

describe("buildMeasurementSettingsCard", () => {
  beforeEach(() => {
    deviceInfo.set(undefined);
  });

  it("returns hidden placeholder for single-meter firmware", () => {
    const T = getStrings();
    deviceInfo.set({
      router_name: "x",
      firmware_version: "1",
      probe_house_name: "",
      probe_second_name: "",
      temperature_label: "",
      capabilities: {
        product_profile: "jsy_mk194_router",
        firmware_capabilities: {
          meter_pack: "jsy_mk194",
          meters: ["JsyMk194"],
        },
      },
    });
    const card = buildMeasurementSettingsCard(
      T,
      { source: "JsyMk194" } as RouterConfig,
      new AbortController().signal,
      document.createElement("div"),
      vi.fn(),
    );
    expect(card.hidden).toBe(true);
    expect(card.className).toContain("settings-measurement-source--fixed");
    expect(card.querySelector(".section__title")).toBeNull();
    expect(card.querySelector("button")).toBeNull();
    expect(card.textContent).not.toContain(T.settings.sectionMeasurement);
  });

  it("openSourceSetupWizard no-ops for single-meter unless edit_pmqtt", async () => {
    const { openSourceSetupWizard } = await import("../../../components/SourceSetupWizard");
    const { openDialog } = await import("../../../components/Dialog");
    deviceInfo.set({
      router_name: "x",
      firmware_version: "1",
      probe_house_name: "",
      probe_second_name: "",
      temperature_label: "",
      capabilities: {
        product_profile: "jsy_mk194_router",
        firmware_capabilities: { meters: ["JsyMk194"] },
      },
    });
    openSourceSetupWizard({
      initialConfig: { source: "JsyMk194" } as RouterConfig,
      signal: new AbortController().signal,
    });
    expect(openDialog).not.toHaveBeenCalled();
  });

  it("opens source wizard on primary button click", async () => {
    const { openSourceSetupWizard } = await import("../../../components/SourceSetupWizard");
    const cfg = { source: "JsyMk194" } as RouterConfig;
    const onSaved = vi.fn();
    const card = buildMeasurementSettingsCard(
      getStrings(),
      cfg,
      new AbortController().signal,
      document.createElement("div"),
      onSaved,
    );
    const buttons = card.querySelectorAll("button");
    expect(buttons.length).toBeGreaterThanOrEqual(2);
    buttons[0]?.click();
    expect(openSourceSetupWizard).toHaveBeenCalledWith(
      expect.objectContaining({ initialConfig: cfg, onSaved }),
    );
    buttons[1]?.click();
    expect(openSourceSetupWizard).toHaveBeenCalledWith(
      expect.objectContaining({ preset: "split_ext" }),
    );
  });
});

describe("buildAdvancedSection", () => {
  const signal = new AbortController().signal;

  beforeEach(() => {
    vi.clearAllMocks();
    apiMock.saveNow.mockResolvedValue(undefined);
    apiMock.resetHistory.mockResolvedValue(undefined);
    apiMock.factoryReset.mockResolvedValue(undefined);
  });

  it("save EEPROM button calls api.saveNow", async () => {
    const section = buildAdvancedSection(
      getStrings(),
      signal,
      document.createElement("p"),
      { el: document.createElement("div") },
      textRow("ntp1", "NTP1", "", ""),
      textRow("ntp2", "NTP2", "", ""),
    );
    const saveBtn = Array.from(section.querySelectorAll("button")).find((b) =>
      b.textContent?.includes(getStrings().settings.saveEeprom),
    );
    saveBtn?.click();
    await vi.waitFor(() => expect(apiMock.saveNow).toHaveBeenCalledWith({ signal }));
  });

  it("factory reset requires confirmation token", async () => {
    const { openDialog } = await import("../../../components/Dialog");
    const { toast } = await import("../../../components/Toast");
    const T = getStrings();
    const section = buildAdvancedSection(
      T,
      signal,
      document.createElement("p"),
      { el: document.createElement("div") },
      textRow("ntp1", "NTP1", "", ""),
      textRow("ntp2", "NTP2", "", ""),
    );
    const factoryBtn = Array.from(section.querySelectorAll("button")).find((b) =>
      b.textContent?.includes(T.settings.factoryReset),
    );
    factoryBtn?.click();
    expect(openDialog).toHaveBeenCalled();
    const dialogOpts = vi.mocked(openDialog).mock.calls.at(-1)?.[0];
    const input = dialogOpts?.body?.querySelector("input") as HTMLInputElement;
    input.value = "wrong";
    await dialogOpts?.actions?.[1]?.onClick?.();
    expect(toast).toHaveBeenCalledWith(T.settings.actionFailed, "error");
    input.value = T.settings.factoryResetToken;
    await dialogOpts?.actions?.[1]?.onClick?.();
    await vi.waitFor(() =>
      expect(apiMock.factoryReset).toHaveBeenCalledWith({ signal }),
    );
  });
});
