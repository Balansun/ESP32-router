import { describe, expect, it, beforeEach } from "vitest";
import { deviceInfo } from "../state/store";
import { buildShell } from "./Shell";

describe("Shell nav capability gating", () => {
  beforeEach(() => {
    document.body.replaceChildren();
    deviceInfo.set(undefined);
  });

  it("hides Actions tab when surplus_regulation is false", () => {
    buildShell();
    deviceInfo.set({
      router_name: "Lab",
      firmware_version: "0.0.0",
      probe_house_name: "",
      probe_second_name: "",
      temperature_label: "",
      capabilities: {
        product_profile: "jsy_mk194_meter",
        firmware_capabilities: { surplus_regulation: false, triac: false },
      },
    });
    const actions = document.querySelector('a.tabbar__item[href$="/actions"]');
    expect(actions).toBeNull();
  });

});
