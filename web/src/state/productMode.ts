import { deviceInfo, signal } from "./store";
import {
  profileVariantByPioEnv,
  type ProfileMatrixVariant,
} from "../utils/profileMatrix";

const STORAGE_KEY = "balansun_product_mode_pio_env";
const DEFAULT_FULL_ENV = "wroom32";

/** Selected catalog env for UI feature gating on full firmware. */
export const productModePioEnv = signal<string | null>(readStoredPioEnv());

function readStoredPioEnv(): string | null {
  if (typeof localStorage === "undefined") return null;
  const v = localStorage.getItem(STORAGE_KEY)?.trim();
  return v || null;
}

export function isFullFirmware(): boolean {
  const caps = deviceInfo.get()?.capabilities;
  const profile = caps?.product_profile;
  const fw = caps?.firmware_capabilities;
  if (profile !== "full_router") return false;
  const pack = fw?.meter_pack;
  return pack === "full" || pack === undefined || pack === "";
}

/** Active profile variant for UI gates (full firmware only). */
export function activeProductModeVariant(): ProfileMatrixVariant | null {
  if (!isFullFirmware()) return null;
  const env = productModePioEnv.get() ?? DEFAULT_FULL_ENV;
  return profileVariantByPioEnv(env) ?? profileVariantByPioEnv(DEFAULT_FULL_ENV) ?? null;
}

export function canSelectProductMode(): boolean {
  return isFullFirmware();
}

/** Sync stored mode after device capabilities load; noop on stripped firmware. */
export function syncProductModeFromDevice(): void {
  if (!isFullFirmware()) {
    productModePioEnv.set(null);
    return;
  }
  const stored = readStoredPioEnv();
  if (stored && profileVariantByPioEnv(stored)) {
    productModePioEnv.set(stored);
    return;
  }
  productModePioEnv.set(DEFAULT_FULL_ENV);
  persistProductMode(DEFAULT_FULL_ENV);
}

export function setProductModePioEnv(pioEnv: string): void {
  if (!canSelectProductMode()) return;
  if (!profileVariantByPioEnv(pioEnv)) return;
  productModePioEnv.set(pioEnv);
  persistProductMode(pioEnv);
}

function persistProductMode(pioEnv: string): void {
  try {
    localStorage.setItem(STORAGE_KEY, pioEnv);
  } catch {
    /* quota / private mode */
  }
}
