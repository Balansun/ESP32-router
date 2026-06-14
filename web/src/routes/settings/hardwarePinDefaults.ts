import type { RouterConfig } from "../../api/types";
import type { BoardEnv } from "../../firmware/boardEnv";

/** PCB default GPIO map (WROOM-32 / WROVER-32 / 32U — classic ESP32). */
export const HARDWARE_PIN_DEFAULTS_WROOM = {
  pin_triac_dim: 22,
  pin_zero_cross: 23,
  pin_uart_rx: 26,
  pin_uart_tx: 27,
  pin_temp: 13,
  pin_analog0: 35,
  pin_analog1: 32,
  pin_analog2: 33,
} as const;

/** PCB default GPIO map (ESP32-S3 DevKit / N16R8). */
export const HARDWARE_PIN_DEFAULTS_S3 = {
  pin_triac_dim: 22,
  pin_zero_cross: 23,
  pin_uart_rx: 4,
  pin_uart_tx: 5,
  pin_temp: 21,
  pin_analog0: 1,
  pin_analog1: 2,
  pin_analog2: 3,
} as const;

export type HardwarePinDefaults = {
  pin_triac_dim: number;
  pin_zero_cross: number;
  pin_uart_rx: number;
  pin_uart_tx: number;
  pin_temp: number;
  pin_analog0: number;
  pin_analog1: number;
  pin_analog2: number;
};

export function hardwarePinDefaultsForBoardEnv(boardEnv: BoardEnv): HardwarePinDefaults {
  return boardEnv === "esp32s3"
    ? { ...HARDWARE_PIN_DEFAULTS_S3 }
    : { ...HARDWARE_PIN_DEFAULTS_WROOM };
}

/** Resolve PCB defaults: firmware export first, then board profile / chip model fallback. */
export function resolveHardwarePinDefaults(
  cfg: RouterConfig,
  boardEnv: BoardEnv,
): HardwarePinDefaults {
  const fallback = hardwarePinDefaultsForBoardEnv(boardEnv);
  return {
    pin_triac_dim: cfg.pin_default_triac_dim ?? fallback.pin_triac_dim,
    pin_zero_cross: cfg.pin_default_zero_cross ?? fallback.pin_zero_cross,
    pin_uart_rx: cfg.pin_default_uart_rx ?? fallback.pin_uart_rx,
    pin_uart_tx: cfg.pin_default_uart_tx ?? fallback.pin_uart_tx,
    pin_temp: cfg.pin_default_temp ?? fallback.pin_temp,
    pin_analog0: cfg.pin_default_analog0 ?? fallback.pin_analog0,
    pin_analog1: cfg.pin_default_analog1 ?? fallback.pin_analog1,
    pin_analog2: cfg.pin_default_analog2 ?? fallback.pin_analog2,
  };
}

export function resolveBoardPinProfile(cfg: RouterConfig, chipModel: string | undefined): BoardEnv {
  const profile = cfg.board_pin_profile;
  if (profile === "esp32s3" || profile === "wroom32") return profile;
  if (chipModel && /s3/i.test(chipModel)) return "esp32s3";
  return "wroom32";
}
