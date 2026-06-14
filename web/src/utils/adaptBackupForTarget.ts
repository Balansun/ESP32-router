import type { DeviceCapabilities, HardwarePinMap, RouterConfig } from "../api/types";
import {
  configForBackupImport,
  ensureRouterConfigPutPayload,
  pickPersistedConfig,
  sanitizeConfigForImport,
} from "../api/configPut";
import { overlayPinMapOnConfig, PERSISTED_PIN_MAP_KEYS } from "../api/pinMapMerge";
import type { BalansunBackup } from "./backupFormat";

export function isSparseBackup(backup: BalansunBackup): boolean {
  const mode = backup.device?.export_mode;
  return mode === "sparse" || mode === "sparse_parts";
}

/** Expand sparse backup to full PUT shape when restoring onto a full-export tier. */
export function expandSparseBackupForImport(backup: BalansunBackup): BalansunBackup {
  return {
    ...backup,
    config: ensureRouterConfigPutPayload(
      sanitizeConfigForImport(backup.config ?? ({} as RouterConfig)),
    ),
  };
}

/**
 * Prepare backup payload for PUT /system/backup on the current device.
 * Sparse files stay sparse on constrained targets; full tiers get a complete config object.
 */
export function adaptBackupForTarget(
  backup: BalansunBackup,
  caps: DeviceCapabilities | undefined,
): BalansunBackup {
  const sparse = isSparseBackup(backup);
  const targetFull = caps?.full_config_export !== false;
  if (sparse && targetFull) {
    return expandSparseBackupForImport(backup);
  }
  if (sparse) {
    return {
      ...backup,
      config: sanitizeConfigForImport(backup.config ?? ({} as RouterConfig)),
    };
  }
  return {
    ...backup,
    config: configForBackupImport(backup.config ?? ({} as RouterConfig)),
  };
}

/** Pin fields stored in backup `config` (restored via PUT /system/pins after monolithic backup). */
export function pinPatchFromBackupConfig(
  cfg: RouterConfig | undefined,
): Partial<HardwarePinMap> | undefined {
  if (!cfg) return undefined;
  const patch: Partial<HardwarePinMap> = {};
  let has = false;
  for (const k of PERSISTED_PIN_MAP_KEYS) {
    const v = cfg[k as keyof RouterConfig];
    if (v !== undefined) {
      (patch as Record<string, unknown>)[k] = v;
      has = true;
    }
  }
  return has ? patch : undefined;
}

/** Client-side export bundle before download (respects target caps for pmqtt). */
export function adaptBackupForExport(
  backup: BalansunBackup,
  caps: DeviceCapabilities | undefined,
  pins?: Partial<HardwarePinMap>,
): BalansunBackup {
  const full = caps?.full_config_export !== false;
  if (full) {
    return {
      ...backup,
      config: pickPersistedConfig(backup.config ?? ({} as RouterConfig), {
        pins,
      }),
    };
  }
  let cfg = sanitizeConfigForImport(backup.config ?? ({} as RouterConfig));
  if (pins) {
    cfg = overlayPinMapOnConfig(cfg, pins);
  }
  if (cfg.source !== "Pmqtt") {
    delete cfg.pmqtt_bindings;
  }
  return { ...backup, config: cfg };
}
