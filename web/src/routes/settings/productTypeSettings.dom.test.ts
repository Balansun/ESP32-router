import { beforeEach, describe, expect, it } from "vitest";
import { getStrings } from "../../i18n";
import catalog from "../../../public/firmware-catalog.json";
import { buildProductTypeSettingsCard } from "./productTypeSettings";
import type { FirmwareCatalog } from "../../utils/productProfile";
import { deviceInfo } from "../../state/store";
import { productModePioEnv } from "../../state/productMode";
import { hasSurplusRegulation } from "../../utils/firmwareCaps";

const CATALOG = catalog as FirmwareCatalog;

describe("buildProductTypeSettingsCard", () => {
  const T = getStrings();

  beforeEach(() => {
    deviceInfo.set(undefined);
    productModePioEnv.set(null);
    localStorage.removeItem("balansun_product_mode_pio_env");
  });

  it("applies mode only when Apply is pressed on full firmware", () => {
    deviceInfo.set({
      router_name: "Balansun",
      firmware_version: "0.1.0",
      probe_house_name: "",
      probe_second_name: "",
      temperature_label: "",
      capabilities: {
        product_profile: "full_router",
        firmware_capabilities: {
          surplus_regulation: true,
          multi_action: true,
          meter_pack: "full",
          meters: ["JsyMk194", "Linky"],
        },
      },
    });
    const gates: boolean[] = [];
    const { card } = buildProductTypeSettingsCard(T, {
      catalog: CATALOG,
      productProfile: "full_router",
      firmwareCapabilities: {
        surplus_regulation: true,
        multi_action: true,
        meter_pack: "full",
      },
      firmwareVersion: "0.1.0",
      onModeChange: () => gates.push(hasSurplusRegulation()),
    });
    const family = card.querySelector("#product-family") as HTMLSelectElement;
    const decl = card.querySelector("#product-declination") as HTMLSelectElement;
    const apply = card.querySelector("#product-mode-apply") as HTMLButtonElement;
    expect(family.disabled).toBe(false);
    expect(decl.disabled).toBe(false);
    expect(card.querySelector(".product-type__ota")?.hidden).toBe(true);

    family.value = "meter";
    family.dispatchEvent(new Event("change"));
    decl.value = "jsy_mk194_meter";
    decl.dispatchEvent(new Event("change"));

    expect(productModePioEnv.get()).not.toBe("jsy_mk194_meter");
    expect(hasSurplusRegulation()).toBe(true);
    expect(apply.disabled).toBe(false);

    apply.click();

    expect(productModePioEnv.get()).toBe("jsy_mk194_meter");
    expect(hasSurplusRegulation()).toBe(false);
    expect(gates.at(-1)).toBe(false);
    expect(apply.disabled).toBe(true);
  });

  it("shows OTA banner on stripped firmware when selection differs", () => {
    const { card } = buildProductTypeSettingsCard(T, {
      catalog: CATALOG,
      productProfile: "jsy_mk194_meter",
      firmwareVersion: "0.1.0",
    });
    const family = card.querySelector("#product-family") as HTMLSelectElement;
    expect(family.disabled).toBe(true);
    family.value = "router";
    family.dispatchEvent(new Event("change"));
    const decl = card.querySelector("#product-declination") as HTMLSelectElement;
    decl.value = "jsy_mk194_router";
    decl.dispatchEvent(new Event("change"));
    const banner = card.querySelector(".product-type__ota") as HTMLElement;
    expect(banner.hidden).toBe(false);
    expect(banner.textContent).toContain("jsy_mk194_router");
  });
});
