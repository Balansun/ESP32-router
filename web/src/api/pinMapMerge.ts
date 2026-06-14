import type { HardwarePinMap, RouterConfig } from "./types";

/** EEPROM / backup pin map keys (subset of RouterConfig). */
export const PERSISTED_PIN_MAP_KEYS = [
  "pin_triac_dim",
  "pin_zero_cross",
  "pin_uart_rx",
  "pin_uart_tx",
  "pin_temp",
  "pin_analog0",
  "pin_analog1",
  "pin_analog2",
  "board_pin_profile",
  "pin_default_triac_dim",
  "pin_default_zero_cross",
  "pin_default_uart_rx",
  "pin_default_uart_tx",
  "pin_default_temp",
  "pin_default_analog0",
  "pin_default_analog1",
  "pin_default_analog2",
  "pin_map_fallback_active",
  "pin_map_reboot_pending",
] as const satisfies readonly (keyof RouterConfig)[];

export type PersistedPinMapKey = (typeof PERSISTED_PIN_MAP_KEYS)[number];

/** Copy defined pin fields from a partial map into config (backup export / restore). */
export function overlayPinMapOnConfig(
  cfg: RouterConfig,
  pins: Partial<HardwarePinMap>,
): RouterConfig {
  const out = { ...cfg };
  for (const k of PERSISTED_PIN_MAP_KEYS) {
    const v = pins[k as keyof HardwarePinMap];
    if (v !== undefined) {
      (out as Record<string, unknown>)[k] = v;
    }
  }
  return out;
}

/** Overlay EEPROM pin map from GET /api/v1/system/pins onto sparse RouterConfig. */
export function mergePinFieldsIntoConfig(cfg: RouterConfig, pins: HardwarePinMap): RouterConfig {
  return {
    ...cfg,
    pin_triac_dim: pins.pin_triac_dim ?? cfg.pin_triac_dim,
    pin_zero_cross: pins.pin_zero_cross ?? cfg.pin_zero_cross,
    pin_uart_rx: pins.pin_uart_rx ?? cfg.pin_uart_rx,
    pin_uart_tx: pins.pin_uart_tx ?? cfg.pin_uart_tx,
    pin_temp: pins.pin_temp ?? cfg.pin_temp,
    pin_analog0: pins.pin_analog0 ?? cfg.pin_analog0,
    pin_analog1: pins.pin_analog1 ?? cfg.pin_analog1,
    pin_analog2: pins.pin_analog2 ?? cfg.pin_analog2,
    pin_map_fallback_active: pins.pin_map_fallback_active ?? cfg.pin_map_fallback_active,
    pin_map_reboot_pending: pins.pin_map_reboot_pending ?? cfg.pin_map_reboot_pending,
    board_pin_profile: pins.board_pin_profile ?? cfg.board_pin_profile,
    pin_default_triac_dim: pins.pin_default_triac_dim ?? cfg.pin_default_triac_dim,
    pin_default_zero_cross: pins.pin_default_zero_cross ?? cfg.pin_default_zero_cross,
    pin_default_uart_rx: pins.pin_default_uart_rx ?? cfg.pin_default_uart_rx,
    pin_default_uart_tx: pins.pin_default_uart_tx ?? cfg.pin_default_uart_tx,
    pin_default_temp: pins.pin_default_temp ?? cfg.pin_default_temp,
    pin_default_analog0: pins.pin_default_analog0 ?? cfg.pin_default_analog0,
    pin_default_analog1: pins.pin_default_analog1 ?? cfg.pin_default_analog1,
    pin_default_analog2: pins.pin_default_analog2 ?? cfg.pin_default_analog2,
  };
}
