// Strongly-typed mirror of the firmware /api/v1 JSON shapes.
// Keep in sync with Api.ino and actions_api.cpp.

import type { PwmMode } from "../routes/settings/pwmFields";

export interface MeterChannel {
  active_import_w: number;
  active_export_w: number;
  apparent_import_va: number;
  apparent_export_va: number;
  energy_day_import_wh: number;
  energy_day_export_wh: number;
  energy_total_import_wh: number;
  energy_total_export_wh: number;
}

export interface RawMeter {
  voltage_house_v: number;
  current_house_a: number;
  pf_house: number;
  voltage_second_v: number;
  current_second_a: number;
  pf_second: number;
  freq_hz: number;
  house_net_power_w: number;
  second_net_power_w: number;
}

export interface TemperatureSlotConfig {
  enabled: boolean;
  label: string;
  /** 16 hex ROM digits; empty = bind by discovery order. */
  address?: string;
}

export interface TemperatureSensorReading {
  slot: number;
  enabled: boolean;
  label?: string | null;
  address?: string | null;
  temperature_c?: number | null;
  valid: boolean;
  primary?: boolean;
}

export interface TemperatureSensorsTelemetry {
  gpio: number;
  max_count: 2;
  discovered_count: number;
  bus_active: boolean;
  sensors: TemperatureSensorReading[];
}

export interface Measurements {
  date_valid: boolean;
  date: string;
  source: string;
  probe_house_name?: string;
  probe_second_name?: string;
  temperature_c?: number;
  temperature_sensors?: TemperatureSensorsTelemetry;
  linky_tariff?: string;
  /** Merged from Tempo RTE when firmware appends tempo fields to measurements. */
  ltarf?: string;
  house: MeterChannel;
  second?: MeterChannel;
  raw_meter: RawMeter;
  diagnostics?: MeasurementDiagnostics;
}

export interface SystemInfo {
  uptime_hours: number;
  wifi_rssi_dbm: number;
  wifi_bssid: string;
  mac: string;
  ssid: string;
  ip: string;
  gateway: string;
  subnet: string;
  dns: string;
  metering_task_ms: [number, number, number];
  loop_task_ms: [number, number, number];
  eeprom_used_percent: number;
  irq_half_period_raw_vs_in?: string;
  /** @deprecated use irq_half_period_raw_vs_in */
  irq_10ms_raw_vs_in?: string;
  configured_frequency_hz?: number;
}

export interface DeviceFirmwareInfo {
  sketch_md5: string;
  sketch_size: number;
  free_sketch_space: number;
  sdk_version: string;
}

export interface DeviceChipInfo {
  model: string;
  revision: number;
  cores: number;
  cpu_mhz: number;
  flash_size: number;
  flash_mhz: number;
  mac: string;
  psram_size?: number;
  psram_free?: number;
}

export interface FirmwareCapabilities {
  surplus_regulation?: boolean;
  triac?: boolean;
  multi_action?: boolean;
  source_test_inject?: boolean;
  meters?: string[];
  meter_pack?: string;
  triac_dimming?: boolean;
}

export interface OutputSuspend {
  active: boolean;
  reason: "none" | "safety_lockout" | "vacation" | "source_stale" | "commissioning";
}

export interface SafetyLockout {
  active: boolean;
  reasons?: Array<"zc_failed" | "triac_failed" | "source_failed">;
}

export type SelfTestSeverity = "ok" | "warning" | "critical";

export type DeviceLifecycleWire =
  | "booting"
  | "configured"
  | "metering"
  | "regulating"
  | "suspended"
  | "commissioning";

export type RegulationMotionWire =
  | "idle"
  | "increasing"
  | "decreasing"
  | "at_minimum"
  | "at_maximum";

export type TriacSyncStateWire =
  | "unsynced"
  | "acquiring"
  | "synced"
  | "fallback_10ms"
  | "not_applicable";

export interface StateOperationalStatus {
  source_health?: number;
  source_stale?: boolean;
  regulation_active?: boolean;
  site_cap_active?: boolean;
  mqtt_connected?: boolean;
  device_lifecycle?: DeviceLifecycleWire;
  regulation_motion?: RegulationMotionWire;
  output_suspend?: OutputSuspend;
  triac_sync_state?: TriacSyncStateWire;
}

export interface DeviceCapabilities {
  tier: "constrained" | "standard" | "extended";
  psram: boolean;
  hist_max_points: number;
  history_days_retained?: number;
  history_days_capacity?: number;
  history_power_window?: "24h" | "48h";
  config_export_max_bytes: number;
  put_body_max_bytes?: number;
  full_config_export: boolean;
  backup_export_mode?: "monolithic" | "parts";
  backup_part_count?: number;
  backup_part_max_bytes?: number;
  product_profile?: string;
  firmware_capabilities?: FirmwareCapabilities;
}

export interface DeviceInfo {
  /** Factory MAC as 12 lowercase hex digits. */
  device_uid: string;
  source: string;
  source_data: string;
  router_name: string;
  firmware_version: string;
  probe_second_name: string;
  probe_house_name: string;
  ext_peer_ip: string;
  ext_peer_port: number;
  ext_peer_path: string;
  temperature_label: string;
  firmware?: DeviceFirmwareInfo;
  chip?: DeviceChipInfo;
  capabilities?: DeviceCapabilities;
}

export interface RouterConfig {
  dhcp_on: boolean;
  ip_fixed: string;
  gateway: string;
  subnet_mask: string;
  dns: string;
  source: string;
  ext_peer_ip: string;
  ext_peer_port: number;
  ext_peer_path: string;
  /** Ext peer wire format (JSON `/api/v1/measurements` only on firmware 0.3+). */
  ext_protocol?: "json";
  enphase_user: string;
  enphase_password: string;
  /** Shelly channel / Enphase meter index; API also accepts `meter_channel`. */
  enphase_serial: string;
  meter_channel?: string;
  mqtt_repeat_sec: number;
  mqtt_ip: string;
  mqtt_port: number;
  mqtt_user: string;
  mqtt_password: string;
  mqtt_prefix: string;
  mqtt_device_name: string;
  router_name: string;
  probe_second_name: string;
  probe_house_name: string;
  temperature_label: string;
  /** @deprecated Prefer temperature_slots[0].label */
  temp_gpio?: number;
  temperature_slots?: TemperatureSlotConfig[];
  calib_u: number;
  calib_i: number;
  /** Source Pmqtt: JSON topic on the MQTT broker. */
  pmqtt_topic: string;
  /** Clés attendues dans le JSON (ex. "Pw" ou "Pw,Pf"). */
  pmqtt_schema: string;
  /** Source Pmqtt: per-metric topic/format/path bindings (preferred over pmqtt_schema). */
  pmqtt_bindings?: PmqttBinding[];
  /** GET-only on constrained tier: bindings live in NVS but may be omitted from JSON. */
  pmqtt_bindings_on_device?: boolean;
  pmqtt_bindings_exported?: boolean;
  /** Serial2 baud for JSY-MK-333. */
  jsy_mk333_serial_baud: number;
  install_country?: string;
  install_country_variant?: string;
  mains_nominal_v?: number;
  mains_frequency_mode?: "auto" | "manual";
  mains_frequency_hz_manual?: 50 | 60;
  mains_frequency_effective_hz?: number;
  mains_frequency_source?: "meter" | "manual" | "fallback";
  mains_frequency_warning?: string | null;
  /** 0 = disable cap on triac_fixed 100% when probe is hot. */
  triac_override_max_temp_c?: number;
  http_cors_enabled?: boolean;
  pwm_gpio?: number;
  pwm_mode?: PwmMode;
  pwm_duty_percent?: number;
  pwm_inverted?: boolean;
  status_led_mode?: StatusLedMode;
  status_led_gpio_activity?: number;
  status_led_gpio_regulation?: number;
  status_led_rgb_gpio?: number;
  status_led_active_low?: boolean;
  status_led_color_activity?: StatusLedColor;
  status_led_color_regulation?: StatusLedColor;
  status_led_color_reboot?: StatusLedColor;
  status_led_color_ap?: StatusLedColor;
  source_data?: string;
  /** Fetch EDF Tempo colors from api-couleur-tempo.fr (non-Linky sources). */
  tempo_rte_enabled?: boolean;
  triac_cal_enabled?: boolean;
  triac_calibration?: { duty_pct: number; measured_w: number }[];
  hunting_reversal_threshold?: number;
  hunting_window_min?: number;
  expert_regulation_mode?: number;
  regulation_gain?: number;
  vacation_enabled?: boolean;
  vacation_end_epoch?: number;
  max_routed_w?: number;
  triac_off_when_source_stale?: boolean;
  triac_backoff_when_heater_idle?: boolean;
  mqtt_json_commands?: boolean;
  /** Routed load kind (surplus regulation target). */
  load_profile?: string;
  action_daily_cap_wh?: number[];
  /** Concrete GPIO numbers; pin_temp may be -1 (disabled). Applied after reboot. */
  pin_triac_dim?: number;
  pin_zero_cross?: number;
  pin_uart_rx?: number;
  pin_uart_tx?: number;
  pin_temp?: number;
  pin_analog0?: number;
  pin_analog1?: number;
  pin_analog2?: number;
  pin_map_fallback_active?: boolean;
  pin_map_reboot_pending?: boolean;
  /** Read-only PCB defaults for this firmware build (`wroom32` or `esp32s3`). */
  board_pin_profile?: "wroom32" | "esp32s3";
  pin_default_triac_dim?: number;
  pin_default_zero_cross?: number;
  pin_default_uart_rx?: number;
  pin_default_uart_tx?: number;
  pin_default_temp?: number;
  pin_default_analog0?: number;
  pin_default_analog1?: number;
  pin_default_analog2?: number;
}

export type StatusLedMode = "off" | "dual_gpio" | "rgb_ws2812";
export type StatusLedTestRole = "activity" | "regulation" | "both" | "reboot" | "ap";
export type StatusLedColor = [number, number, number];

export interface StatusLedTestRequest extends Partial<RouterConfig> {
  role: StatusLedTestRole;
  duration_ms?: number;
}

export interface StatusLedTestResponse {
  ok: boolean;
  role: StatusLedTestRole;
  duration_ms: number;
}

export type PmqttBindingFormat = "plain" | "json" | "snapshot";

export interface PmqttBinding {
  metric: string;
  topic: string;
  format: PmqttBindingFormat;
  /** Dot path (`house.Pw`) or JSONPath style (`$.house.Pw`). */
  path?: string;
  enabled?: boolean;
}

export interface TempoTariffStatus {
  enabled: boolean;
  ltarf?: string;
  tomorrow_stge?: string;
  today_color?: string;
  tomorrow_color?: string;
  tariff_code?: number;
  ltarf_bin?: number;
  stale?: boolean;
  last_fetch_epoch?: number;
}

export interface ConfigEnvelope {
  schema_version: number;
  config: RouterConfig;
}

/** GET/PUT /api/v1/system/pins — authoritative GPIO map (including on constrained config export). */
export interface HardwarePinMap {
  pin_triac_dim: number;
  pin_zero_cross: number;
  pin_uart_rx: number;
  pin_uart_tx: number;
  pin_temp: number;
  pin_analog0: number;
  pin_analog1: number;
  pin_analog2: number;
  pin_map_fallback_active?: boolean;
  pin_map_reboot_pending?: boolean;
  board_pin_profile?: "wroom32" | "esp32s3";
  pin_default_triac_dim?: number;
  pin_default_zero_cross?: number;
  pin_default_uart_rx?: number;
  pin_default_uart_tx?: number;
  pin_default_temp?: number;
  pin_default_analog0?: number;
  pin_default_analog1?: number;
  pin_default_analog2?: number;
}

export interface PinMapPutResponse extends ApiOk {
  reboot_required?: boolean;
  eeprom_bytes?: number;
}

export interface LiveActionSlot {
  index: number;
  title: string;
  triac_open_percent?: number;
  on?: boolean;
}

export interface ActionsLive {
  temperature_c: number;
  temperature_sensors?: TemperatureSensorsTelemetry;
  source: string;
  ext_peer_ip: string;
  ext_peer_port: number;
  ext_peer_path: string;
  active_actions_count: number;
  active_slots: LiveActionSlot[];
}

export type PeriodMode = "off" | "on" | "power";

export interface ActionPeriod {
  mode: PeriodMode;
  hour_end: number;
  power_min_w: number;
  power_max_w: number;
  temp_inf_c: number;
  temp_sup_c: number;
}

export type ActionKind = "triac" | "local_gpio" | "remote_http";

export interface ActionConfig {
  index: number;
  title: string;
  kind: ActionKind;
  host?: string;
  port?: number;
  path_on?: string;
  path_off?: string;
  triac_sensitivity?: number;
  /** Regulation mode 0..5 on firmware `Actif`. */
  regulation_mode?: number;
  ki?: number;
  kp?: number;
  kd?: number;
  pid_enabled?: boolean;
  repeat_sec: number;
  tempo_sec: number;
  periods: ActionPeriod[];
}

export interface ActionsConfigEnvelope {
  schema_version: number;
  nb_actions: number;
  actions: ActionConfig[];
}

export interface HistoryPower {
  source: string;
  window: string;
  max_points: number;
  sample_period_s: number;
  temperature_now_c?: number;
  house_active_w: number[];
  house_apparent_va?: number[];
  triac_active_w: number[];
  triac_apparent_va?: number[];
  temperature_series_c?: number[];
}

export interface HistoryEnergyDaily {
  delta_wh_per_day: number[];
  count: number;
  total_count?: number;
  offset?: number;
  limit?: number;
  has_more?: boolean;
  idx_prom_du_jour: number;
  days_capacity?: number;
  days_retained?: number;
  /** Today's calendar date (device local TZ) when NTP sync is valid. */
  reference_date_iso?: string;
  /** ISO dates aligned with `delta_wh_per_day` (index 0 = oldest). */
  day_dates_iso?: string[];
  import_wh_per_day?: number[];
  export_wh_per_day?: number[];
  ch1_import_wh_per_day?: number[];
  ch1_export_wh_per_day?: number[];
  ch2_import_wh_per_day?: number[];
  ch2_export_wh_per_day?: number[];
  items?: Array<{
    date_iso: string;
    delta_wh: number;
    ch1_import_wh: number;
    ch1_export_wh: number;
    ch2_import_wh: number;
    ch2_export_wh: number;
  }>;
}

export interface ApiOk {
  ok: true;
  [k: string]: unknown;
}

/** `POST /api/v1/mqtt/test` — synchronous broker connect probe (does not save config). */
export interface MqttTestResponse {
  ok: boolean;
  mqtt_connected: boolean;
  error_code: number;
  message: string;
}

/** Response from `POST /api/v1/firmware/ota` on success. */
export interface FirmwareOtaResponse {
  ok: boolean;
  message?: string;
}

export interface HttpAuthInfo {
  enabled: boolean;
  username: string;
}

/** `GET /api/v1/public` — no secrets; readable without HTTP API password. */
export interface PublicInfo {
  http_auth: HttpAuthInfo;
  device: {
    router_name: string;
    firmware_version: string;
    source_configured: boolean;
  };
  wifi: {
    mode: "ap" | "sta";
    connected: boolean;
    /** True while captive-portal soft-AP is active (even if STA is also up). */
    setup_ap?: boolean;
  };
}

export interface HttpAuthPutResponse extends ApiOk {
  enabled: boolean;
  eeprom_bytes?: number;
}

export interface AuthTokenInfo {
  id: number;
  label: string;
}

export interface AuthTokenCreateResponse extends ApiOk {
  id: number;
  label: string;
  token: string;
}

export interface ArduinoOtaInfo {
  password_set: boolean;
}

export interface ArduinoOtaPutResponse {
  ok: true;
  eeprom_bytes: number;
  message?: string;
}

export type OverrideState = "auto" | "on" | "off" | "triac_fixed";

export interface ActionOverride {
  index: number;
  state: OverrideState;
  triac_open_percent: number;
  sticky: boolean;
  expires_in_s: number;
}

export interface OverrideRequest {
  state: OverrideState;
  triac_open_percent?: number;
  duration_s?: number;
}

export interface StateEnvelope {
  measurements: Measurements;
  actions_live: ActionsLive;
  triac_open_percent: number;
  heater_load_backoff_active?: boolean;
  temperature_c: number;
  time: string;
  date_valid: boolean;
  source: string;
  override_summary: ActionOverride[];
  status?: StateOperationalStatus;
}

export interface HealthSelfTest {
  pending?: boolean;
  skipped?: boolean;
  running?: boolean;
  last_run_epoch?: number;
  safety_lockout_active?: boolean;
  safety_lockout_reasons?: string[];
  severity?: {
    zc?: SelfTestSeverity;
    triac?: SelfTestSeverity;
    source?: SelfTestSeverity;
  };
  results?: {
    zc_ok?: boolean;
    triac_ok?: boolean;
    source_ok?: boolean;
    zc_edges_per_sec?: number;
  };
}

export type HardwarePresenceState = "ok" | "missing" | "paused" | "not_applicable";

export interface HardwarePresenceItem {
  id: string;
  role: string;
  state: HardwarePresenceState;
  gpio?: number;
  gpio_rx?: number;
  gpio_tx?: number;
  detail?: string;
}

export interface HardwarePresenceInfo {
  items?: HardwarePresenceItem[];
}

export interface HealthInfo {
  ok: boolean;
  uptime_s: number;
  telemetry_ready?: boolean;
  /** Present on /health when measurements route is not ready yet (bench without meter). */
  temperature_c?: number;
  temperature_sensors?: TemperatureSensorsTelemetry;
  source_ok: boolean;
  date_valid: boolean;
  mqtt_connected: boolean;
  free_heap: number;
  wifi_connected: boolean;
  pin_map_fallback_active?: boolean;
  pin_map_reboot_pending?: boolean;
  device_lifecycle?: DeviceLifecycleWire;
  product_profile?: string;
  output_suspend?: OutputSuspend;
  safety_lockout?: SafetyLockout;
  self_test?: HealthSelfTest;
  hardware?: HardwarePresenceInfo;
}

export interface MeasurementDiagnostics {
  adc_clipping?: boolean;
  regulation_hunting?: boolean;
}

export interface ConfigAuditEntry {
  ts_ms: number;
  route: string;
  keys?: string[];
}

export interface ConfigAuditEnvelope {
  entries: ConfigAuditEntry[];
}

export interface SourcesInfo {
  supported: string[];
  current: string;
  current_data: string;
}

export interface WifiInfo {
  ssid: string;
  /** Present on authenticated GET (backup export). */
  password?: string;
  mode: "sta" | "ap";
  connected: boolean;
  setup_ap?: boolean;
  rssi: number;
  ip: string;
}

export interface WifiScanResult {
  scanning: boolean;
  networks: Array<{ ssid: string; rssi: number; channel: number; secure: boolean }>;
}

export interface TimeInfo {
  tz: string;
  ntp1: string;
  ntp2: string;
  date_valid: boolean;
  now: string;
}

export interface SourceDiagnostics {
  source: string;
  source_data: string;
  date: string;
  date_valid: boolean;
  temperature_c: number;
  frequency_hz: number;
  voltage_house_v: number;
  current_house_a: number;
  pf_house: number;
  voltage_second_v: number;
  current_second_a: number;
  pf_second: number;
  uxi_waveform?: { volt_m: number[]; amp_m: number[] };
  linky?: {
    ltarf: string;
    idx_raw: number;
    tail: string;
    tail_len: number;
    linky_eait_from_tic?: boolean;
    linky_sinsti_seen?: boolean;
    cacsi_no_export?: boolean;
  };
  diagnostics?: {
    last_poll_ok?: boolean;
    last_error?: string;
    active_source?: string;
    meter?: string;
    source_data?: string;
    poll_period_ms?: number;
    last_poll_ms_ago?: number;
    protocol_used?: string;
    ext_protocol?: string;
    mains_frequency_hz?: number;
    mains_frequency_source?: string;
    transport_last_poll_ms_ago?: number;
    health_score?: number;
    health_score_factors?: { freshness?: number; poll_ok?: number; streak?: number };
    adc_clipping?: boolean;
    regulation_hunting?: boolean;
  };
  enphase?: {
    user: string;
    serial: string;
    has_user: boolean;
    has_session: boolean;
    has_token: boolean;
    pact_prod_w: number;
    pact_conso_w: number;
  };
  shelly_em?: { raw: string; poll_count: number };
  smartg?: { raw: string };
  ext?: {
    ext_peer_ip: string;
    ext_peer_port: number;
    ext_peer_path: string;
    ext_protocol?: string;
    last_poll_ok: boolean;
    last_poll_ms_ago: number;
    last_error: string;
    last_frame_preview: string;
    protocol_used?: string;
  };
  pmqtt?: {
    bindings?: Array<{
      metric: string;
      topic: string;
      ok: boolean;
      value?: number;
      last_error?: string;
      last_rx_ms_ago?: number;
    }>;
  };
}

export interface PmqttPreviewResponse {
  ok: boolean;
  results: Array<{
    metric: string;
    topic: string;
    format: PmqttBindingFormat;
    path?: string;
    ok: boolean;
    value?: number;
    display?: string;
    age_ms?: number;
    raw_snippet?: string;
    error?: string;
  }>;
}
