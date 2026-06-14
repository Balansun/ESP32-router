import { describe, expect, it, vi } from "vitest";
import { en } from "../src/i18n/locales/en";
import {
  applyInstallCountryToConfig,
  buildInstallCountrySection,
  readInstallCountryState,
} from "../src/components/InstallCountryFields";
import type { RouterConfig } from "../src/api/types";

describe("InstallCountryFields", () => {
  it("applyInstallCountryToConfig uses row defaults when not ZZ", () => {
    const cfg = { install_country: "FR" } as RouterConfig;
    const st = readInstallCountryState(cfg);
    const next = applyInstallCountryToConfig(cfg, st);
    expect(next.mains_nominal_v).toBeGreaterThan(0);
  });

  it("applyInstallCountryToConfig keeps manual values when ZZ", () => {
    const cfg = { install_country: "ZZ", mains_nominal_v: 127 } as RouterConfig;
    const st = readInstallCountryState(cfg);
    st.nominalV = 127;
    const next = applyInstallCountryToConfig(cfg, st);
    expect(next.mains_nominal_v).toBe(127);
  });

  it("defaults split variant when country has variants", () => {
    const section = buildInstallCountrySection(
      en,
      "en",
      readInstallCountryState({ install_country: "BR", install_country_variant: "" } as RouterConfig),
      () => {},
    );
    expect(section.getState().variant).toBeTruthy();
  });

  it("readInstallCountryState and applyInstallCountryToConfig", () => {
    const cfg = {
      install_country: "DE",
      install_country_variant: "",
      mains_frequency_mode: "manual",
      mains_frequency_hz_manual: 60,
      mains_nominal_v: 230,
    } as RouterConfig;
    const st = readInstallCountryState(cfg);
    expect(st.country).toBe("DE");
    const next = applyInstallCountryToConfig(cfg, { ...st, country: "FR" });
    expect(next.install_country).toBe("FR");
    expect(next.mains_nominal_v).toBeGreaterThan(0);
  });

  it("buildInstallCountrySection updates state on change", () => {
    const onChange = vi.fn();
    const section = buildInstallCountrySection(
      en,
      "en",
      readInstallCountryState({ install_country: "FR" } as RouterConfig),
      onChange,
    );
    section.setFrequencyWarning("warn");
    expect(section.section.querySelector(".field__error")?.hidden).toBe(false);
    section.setFrequencyWarning(null);
    expect(section.section.querySelector(".field__error")?.hidden).toBe(true);
    const select = section.section.querySelector(
      "#install_country",
    ) as HTMLSelectElement;
    select.value = "DE";
    select.dispatchEvent(new Event("change", { bubbles: true }));
    expect(onChange).toHaveBeenCalled();
    expect(section.getState().country).toBe("DE");
  });

  it("ZZ country enables edit defaults", () => {
    const section = buildInstallCountrySection(
      en,
      "en",
      readInstallCountryState({ install_country: "ZZ" } as RouterConfig),
      () => {},
    );
    expect(section.getState().editDefaults).toBe(true);
    const edit = section.section.querySelector("#edit_mains_defaults") as HTMLInputElement;
    edit.checked = false;
    edit.dispatchEvent(new Event("change", { bubbles: true }));
    expect(section.getState().editDefaults).toBe(false);
  });

  it("switching to ZZ from FR enables edit defaults", () => {
    const section = buildInstallCountrySection(
      en,
      "en",
      readInstallCountryState({ install_country: "FR" } as RouterConfig),
      () => {},
    );
    const select = section.section.querySelector("#install_country") as HTMLSelectElement;
    select.value = "ZZ";
    select.dispatchEvent(new Event("change", { bubbles: true }));
    expect(section.getState().country).toBe("ZZ");
    expect(section.getState().editDefaults).toBe(true);
  });

  it("Japan split variant uses split note label", () => {
    const section = buildInstallCountrySection(
      en,
      "en",
      readInstallCountryState({ install_country: "JP", install_country_variant: "" } as RouterConfig),
      () => {},
    );
    expect(section.getState().variant).toBeTruthy();
    const variant = section.section.querySelector("#install_country_variant") as HTMLSelectElement;
    expect(variant).toBeTruthy();
  });

  it("applyInstallCountryToConfig with editDefaults on FR keeps nominalV", () => {
    const cfg = { install_country: "FR", mains_nominal_v: 230 } as RouterConfig;
    const st = readInstallCountryState(cfg);
    st.editDefaults = true;
    st.nominalV = 127;
    const next = applyInstallCountryToConfig(cfg, st);
    expect(next.mains_nominal_v).toBe(127);
  });

  it("readInstallCountryState uses 50 Hz when manual field is not 60", () => {
    const st = readInstallCountryState({
      install_country: "FR",
      mains_frequency_hz_manual: 50,
    } as RouterConfig);
    expect(st.manualHz).toBe(50);
  });

  it("auto frequency mode on FR disables manual Hz until edit defaults", () => {
    const section = buildInstallCountrySection(
      en,
      "en",
      readInstallCountryState({
        install_country: "FR",
        mains_frequency_mode: "auto",
      } as RouterConfig),
      () => {},
    );
    const hz = section.section.querySelector("#mains_frequency_hz_manual") as HTMLSelectElement;
    expect(hz.disabled).toBe(true);
    const freq = section.section.querySelector("#mains_frequency_mode") as HTMLSelectElement;
    freq.value = "auto";
    freq.dispatchEvent(new Event("change", { bubbles: true }));
    expect(section.getState().frequencyMode).toBe("auto");
  });

  it("nominal input falls back to 230 when empty", () => {
    const section = buildInstallCountrySection(
      en,
      "en",
      readInstallCountryState({ install_country: "ZZ" } as RouterConfig),
      () => {},
    );
    const nominal = section.section.querySelector("#mains_nominal_v") as HTMLInputElement;
    nominal.value = "";
    nominal.dispatchEvent(new Event("input", { bubbles: true }));
    expect(section.getState().nominalV).toBe(230);
  });

  it("switching to FR hides variant row", () => {
    const section = buildInstallCountrySection(
      en,
      "en",
      readInstallCountryState({ install_country: "BR", install_country_variant: "127" } as RouterConfig),
      () => {},
    );
    const select = section.section.querySelector("#install_country") as HTMLSelectElement;
    select.value = "FR";
    select.dispatchEvent(new Event("change", { bubbles: true }));
    expect(section.getState().variant).toBe("");
    expect(section.section.querySelector("#install_country_variant")?.closest(".field")?.hidden).toBe(true);
  });

  it("manual Hz 50 branch on ZZ country", () => {
    const section = buildInstallCountrySection(
      en,
      "en",
      readInstallCountryState({ install_country: "ZZ" } as RouterConfig),
      () => {},
    );
    const hz = section.section.querySelector("#mains_frequency_hz_manual") as HTMLSelectElement;
    hz.value = "50";
    hz.dispatchEvent(new Event("change", { bubbles: true }));
    expect(section.getState().manualHz).toBe(50);
  });

  it("refreshSummary uses editDefaults nominal on non-ZZ country", () => {
    const section = buildInstallCountrySection(
      en,
      "en",
      {
        country: "FR",
        variant: "",
        frequencyMode: "manual",
        manualHz: 50,
        nominalV: 127,
        editDefaults: true,
      },
      () => {},
    );
    expect(section.section.textContent).toContain("127");
  });

  it("readInstallCountryState defaults empty install_country to FR", () => {
    expect(readInstallCountryState({} as RouterConfig).country).toBe("FR");
  });

  it("refreshSummary uses manual nominal when lookup row missing", () => {
    const section = buildInstallCountrySection(
      en,
      "en",
      {
        country: "XX",
        variant: "",
        frequencyMode: "auto",
        manualHz: 60,
        nominalV: 117,
        editDefaults: false,
      },
      () => {},
    );
    expect(section.section.textContent).toContain("117");
    expect(section.section.textContent).toContain("60");
  });

  it("variant label uses installVariant when split note string is empty", () => {
    const T = {
      ...en,
      settings: { ...en.settings, countrySplitBrazil: "" },
    };
    const section = buildInstallCountrySection(
      T,
      "en",
      readInstallCountryState({
        install_country: "BR",
        install_country_variant: "127",
      } as RouterConfig),
      () => {},
    );
    const variantField = section.section
      .querySelector("#install_country_variant")
      ?.closest(".field");
    expect(variantField?.textContent).toContain(T.settings.installVariant);
  });

  it("variant label falls back when split note resolves to non-string", () => {
    const T = {
      ...en,
      settings: {
        ...en.settings,
        countrySplitBrazil: 42 as unknown as string,
      },
    };
    const section = buildInstallCountrySection(
      T,
      "en",
      readInstallCountryState({
        install_country: "BR",
        install_country_variant: "127",
      } as RouterConfig),
      () => {},
    );
    const variantField = section.section
      .querySelector("#install_country_variant")
      ?.closest(".field");
    expect(variantField?.textContent).toContain("settings.countrySplitBrazil");
  });

  it("applyInstallCountryToConfig without lookup row keeps manual fields when ZZ", () => {
    const cfg = { install_country: "ZZ", mains_nominal_v: 127 } as RouterConfig;
    const st = readInstallCountryState(cfg);
    st.nominalV = 200;
    st.manualHz = 60;
    const next = applyInstallCountryToConfig(cfg, st);
    expect(next.mains_nominal_v).toBe(200);
    expect(next.mains_frequency_hz_manual).toBe(60);
  });

  it("Brazil variant and manual frequency update state", () => {
    const onChange = vi.fn();
    const section = buildInstallCountrySection(
      en,
      "en",
      readInstallCountryState({ install_country: "BR", install_country_variant: "127" } as RouterConfig),
      onChange,
    );
    const variant = section.section.querySelector("#install_country_variant") as HTMLSelectElement;
    expect(variant).toBeTruthy();
    variant.value = "220";
    variant.dispatchEvent(new Event("change", { bubbles: true }));
    expect(section.getState().variant).toBe("220");

    const freq = section.section.querySelector("#mains_frequency_mode") as HTMLSelectElement;
    freq.value = "manual";
    freq.dispatchEvent(new Event("change", { bubbles: true }));
    expect(section.getState().frequencyMode).toBe("manual");

    const hz = section.section.querySelector("#mains_frequency_hz_manual") as HTMLSelectElement;
    hz.value = "60";
    hz.dispatchEvent(new Event("change", { bubbles: true }));
    expect(section.getState().manualHz).toBe(60);

    const nominal = section.section.querySelector("#mains_nominal_v") as HTMLInputElement;
    nominal.value = "127";
    nominal.dispatchEvent(new Event("input", { bubbles: true }));
    expect(section.getState().nominalV).toBe(127);
  });
});
