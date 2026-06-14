import { describe, expect, it } from "vitest";
import {
  ensureRouterConfigPutPayload,
  PERSISTED_CONFIG_KEYS,
  pickPersistedConfig,
} from "../src/api/configPut";
import type { RouterConfig } from "../src/api/types";

describe("ensureRouterConfigPutPayload", () => {
  it("fills keys omitted by JSON.stringify(undefined)", () => {
    const partial = {
      dhcp_on: false,
      ip_fixed: "192.168.1.10",
      gateway: "192.168.1.1",
      subnet_mask: "255.255.255.0",
      dns: "192.168.1.1",
      source: "JsyMk194",
      ext_peer_ip: "0.0.0.0",
      mqtt_repeat_sec: 60,
      mqtt_ip: "192.168.1.2",
      mqtt_port: 1883,
      mqtt_user: "",
      mqtt_password: "",
      mqtt_prefix: "home",
      mqtt_device_name: "router",
      router_name: "Test",
      probe_second_name: "Tank",
      probe_house_name: "House",
      temperature_label: "Outdoor",
      calib_u: 1000,
      calib_i: 1000,
      install_country: "FR",
    } as RouterConfig;

    const out = ensureRouterConfigPutPayload(partial);
    expect(out.install_country_variant).toBe("");
    expect(out.ext_protocol).toBe("json");
    expect(out.ext_peer_path).toBe("/api/v1/measurements");
    expect(out.pmqtt_topic).toBe("");
    expect(out.http_cors_enabled).toBe(false);
  });

  it("preserves 60 Hz manual mains frequency", () => {
    const out = ensureRouterConfigPutPayload({
      source: "JsyMk194",
      mains_frequency_hz_manual: 60,
    } as RouterConfig);
    expect(out.mains_frequency_hz_manual).toBe(60);
  });

  it("maps NaN pwm_gpio to -1", () => {
    const out = ensureRouterConfigPutPayload({
      source: "JsyMk194",
      pwm_gpio: Number.NaN,
    } as RouterConfig);
    expect(out.pwm_gpio).toBe(-1);
    expect(JSON.stringify({ config: out })).not.toContain("undefined");
  });

  it("pickPersistedConfig defaults action_daily_cap_wh to empty array", () => {
    const out = pickPersistedConfig({ source: "JsyMk194" } as RouterConfig);
    expect(out.action_daily_cap_wh).toEqual([]);
    expect(PERSISTED_CONFIG_KEYS).toContain("action_daily_cap_wh");
  });
});
