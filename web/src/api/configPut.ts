import type { HardwarePinMap, RouterConfig } from "./types";
import { overlayPinMapOnConfig } from "./pinMapMerge";

/** Defaults aligned with firmware `balansun_globals` / `api_append_config_object`. */
const FIRMWARE_PUT_DEFAULTS: RouterConfig = {
  dhcp_on: true,
  ip_fixed: "0.0.0.0",
  gateway: "0.0.0.0",
  subnet_mask: "255.255.255.0",
  dns: "0.0.0.0",
  source: "JsyMk194",
  ext_peer_ip: "0.0.0.0",
  ext_peer_port: 80,
  ext_peer_path: "/api/v1/measurements",
  ext_protocol: "json",
  enphase_user: "",
  enphase_password: "",
  enphase_serial: "",
  meter_channel: "",
  mqtt_repeat_sec: 0,
  mqtt_ip: "0.0.0.0",
  mqtt_port: 1883,
  mqtt_user: "",
  mqtt_password: "",
  mqtt_prefix: "balansun",
  mqtt_device_name: "balansun",
  router_name: "Balansun",
  probe_second_name: "Second channel",
  probe_house_name: "House metering",
  temperature_label: "temperature_c",
  calib_u: 1000,
  calib_i: 1000,
  pmqtt_topic: "",
  pmqtt_schema: "Pw",
  jsy_mk333_serial_baud: 9600,
  install_country: "FR",
  install_country_variant: "",
  mains_nominal_v: 230,
  mains_frequency_mode: "auto",
  mains_frequency_hz_manual: 50,
  triac_override_max_temp_c: 0,
  http_cors_enabled: false,
};

/** Keys required on firmware full PUT — keep in sync with `keys[]` in `config_apply_from_json`. */
export const FIRMWARE_PUT_REQUIRED_KEYS = Object.keys(
  FIRMWARE_PUT_DEFAULTS,
) as (keyof RouterConfig)[];

/** EEPROM fields applied on PATCH / included in backup `config`. */
export const PERSISTED_CONFIG_OPTIONAL_KEYS = [
  "pwm_gpio",
  "pwm_mode",
  "pwm_duty_percent",
  "pwm_inverted",
  "status_led_mode",
  "status_led_gpio_activity",
  "status_led_gpio_regulation",
  "status_led_rgb_gpio",
  "status_led_active_low",
  "status_led_color_activity",
  "status_led_color_regulation",
  "status_led_color_reboot",
  "status_led_color_ap",
  "tempo_rte_enabled",
  "expert_regulation_mode",
  "regulation_gain",
  "triac_cal_enabled",
  "triac_calibration",
  "hunting_reversal_threshold",
  "hunting_window_min",
  "vacation_enabled",
  "vacation_end_epoch",
  "max_routed_w",
  "mqtt_json_commands",
  "triac_off_when_source_stale",
  "triac_backoff_when_heater_idle",
  "action_daily_cap_wh",
] as const satisfies readonly (keyof RouterConfig)[];

export type PersistedConfigOptionalKey = (typeof PERSISTED_CONFIG_OPTIONAL_KEYS)[number];

/** Derived at runtime — not written back on import. */
export const READ_ONLY_CONFIG_KEYS = [
  "mains_frequency_effective_hz",
  "mains_frequency_source",
  "mains_frequency_warning",
] as const satisfies readonly (keyof RouterConfig)[];

export const PERSISTED_CONFIG_KEYS = [
  ...FIRMWARE_PUT_REQUIRED_KEYS,
  ...PERSISTED_CONFIG_OPTIONAL_KEYS,
] as const;

export type PersistedConfigKey = (typeof PERSISTED_CONFIG_KEYS)[number];

/**
 * Full PUT must include every key the firmware validates; `undefined` is stripped by JSON.stringify.
 */
export function ensureRouterConfigPutPayload(cfg: RouterConfig): RouterConfig {
  const channel = cfg.meter_channel ?? cfg.enphase_serial ?? "";
  const out: RouterConfig = {
    ...FIRMWARE_PUT_DEFAULTS,
    ...cfg,
    install_country_variant: cfg.install_country_variant ?? "",
    mains_frequency_hz_manual:
      cfg.mains_frequency_hz_manual === 60 ? 60 : 50,
    meter_channel: channel,
    enphase_serial: cfg.enphase_serial ?? channel,
  };
  if (typeof out.pwm_gpio === "number" && Number.isNaN(out.pwm_gpio)) {
    out.pwm_gpio = -1;
  }
  return out;
}

const BACKUP_OPTIONAL_DEFAULTS: Pick<RouterConfig, PersistedConfigOptionalKey> = {
  pwm_gpio: -1,
  pwm_mode: "off",
  pwm_duty_percent: 0,
  pwm_inverted: false,
  status_led_mode: "dual_gpio",
  status_led_gpio_activity: 18,
  status_led_gpio_regulation: 19,
  status_led_rgb_gpio: -1,
  status_led_active_low: false,
  status_led_color_activity: [255, 180, 0],
  status_led_color_regulation: [0, 255, 0],
  status_led_color_reboot: [255, 0, 0],
  status_led_color_ap: [102, 0, 255],
  tempo_rte_enabled: false,
  expert_regulation_mode: 0,
  regulation_gain: 1,
  triac_cal_enabled: false,
  triac_calibration: [
    { duty_pct: 0, measured_w: 0 },
    { duty_pct: 0, measured_w: 0 },
    { duty_pct: 0, measured_w: 0 },
  ],
  hunting_reversal_threshold: 3,
  hunting_window_min: 2,
  vacation_enabled: false,
  vacation_end_epoch: 0,
  max_routed_w: 0,
  mqtt_json_commands: false,
  triac_off_when_source_stale: false,
  triac_backoff_when_heater_idle: false,
  action_daily_cap_wh: [],
};

/** Remove derived frequency fields before PUT import / backup export. */
export function sanitizeConfigForImport(cfg: RouterConfig): RouterConfig {
  const out = { ...cfg };
  for (const k of READ_ONLY_CONFIG_KEYS) {
    delete out[k];
  }
  return out;
}

/**
 * Full PUT payload for backup restore — always sends `pmqtt_bindings` when source is Pmqtt
 * (firmware only updates bindings when the key is present on PUT).
 */
export function configForBackupImport(
  cfg: RouterConfig,
  opts?: { sparse?: boolean },
): RouterConfig {
  if (opts?.sparse) {
    return sanitizeConfigForImport(cfg);
  }
  const out = ensureRouterConfigPutPayload(sanitizeConfigForImport(cfg));
  if (out.source === "Pmqtt") {
    out.pmqtt_bindings = Array.isArray(cfg.pmqtt_bindings) ? cfg.pmqtt_bindings : [];
  }
  return out;
}

/** Copy EEPROM-backed config keys for backup export (drops read-only GET fields). */
export function pickPersistedConfig(
  cfg: RouterConfig,
  opts?: { includePmqttBindings?: boolean; pins?: Partial<HardwarePinMap> },
): RouterConfig {
  const includePmqtt = opts?.includePmqttBindings !== false;
  const base = ensureRouterConfigPutPayload(sanitizeConfigForImport(cfg));
  const out: RouterConfig = { ...base, ...BACKUP_OPTIONAL_DEFAULTS };
  for (const k of PERSISTED_CONFIG_OPTIONAL_KEYS) {
    if (k in cfg && cfg[k] !== undefined) {
      (out as unknown as Record<string, unknown>)[k] = cfg[k];
    }
  }
  if (includePmqtt && cfg.source === "Pmqtt") {
    out.pmqtt_bindings = Array.isArray(cfg.pmqtt_bindings) ? cfg.pmqtt_bindings : [];
  } else if (includePmqtt && Array.isArray(cfg.pmqtt_bindings)) {
    out.pmqtt_bindings = cfg.pmqtt_bindings;
  } else {
    delete out.pmqtt_bindings;
  }
  const channel = cfg.meter_channel ?? cfg.enphase_serial ?? "";
  out.meter_channel = channel;
  out.enphase_serial = cfg.enphase_serial ?? channel;
  if (opts?.pins) {
    return overlayPinMapOnConfig(out, opts.pins);
  }
  return out;
}

