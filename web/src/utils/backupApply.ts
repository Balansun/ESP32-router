import { api } from "../api/client";
import { isAuthExemptPath } from "../auth/httpAuthGate";
import type { DeviceCapabilities } from "../api/types";
import { deviceInfo } from "../state/store";
import { toast } from "../components/Toast";
import { openDialog } from "../components/Dialog";
import { getStrings } from "../i18n";
import { h } from "./dom";
import type { BalansunBackup } from "./backupFormat";
import {
  BACKUP_SCHEMA_VERSION,
  detectBackupPartFromJson,
  parseBackupJson,
} from "./backupFormat";
import { adaptBackupForTarget, isSparseBackup, pinPatchFromBackupConfig } from "./adaptBackupForTarget";

const PART_ORDER = ["network", "config", "actions"] as const;

export async function applyBackup(
  backup: BalansunBackup,
  signal?: AbortSignal,
  caps?: DeviceCapabilities,
): Promise<"ok" | "config_failed" | "actions_failed"> {
  const T = getStrings();
  const omitAuth =
    typeof location !== "undefined" && isAuthExemptPath(location.pathname || "/");
  const opts = { signal, retry: 0 as const, omitAuth };
  const targetCaps = caps ?? deviceInfo.get()?.capabilities;
  const adapted = adaptBackupForTarget(backup, targetCaps);

  const payload: BalansunBackup = {
    backupSchemaVersion: BACKUP_SCHEMA_VERSION,
    exportedAt: adapted.exportedAt,
    ...(adapted.device !== undefined ? { device: adapted.device } : {}),
    ...(adapted.config !== undefined ? { config: adapted.config } : {}),
    ...(adapted.actions !== undefined ? { actions: adapted.actions } : {}),
    ...(adapted.time !== undefined ? { time: adapted.time } : {}),
    wifi: adapted.wifi!,
    ...(adapted.api !== undefined ? { api: adapted.api } : {}),
  };

  try {
    await api.putSystemBackup(payload, opts);
    const pinPatch = pinPatchFromBackupConfig(adapted.config);
    if (pinPatch) {
      await api.putSystemPins(pinPatch, opts);
    }
  } catch (e) {
    const networkLost =
      e instanceof TypeError ||
      (e instanceof Error && /failed to fetch|networkerror|load failed/i.test(e.message));
    if (networkLost) {
      toast(T.wifi.saveApLikelyOk, "success", 8000);
      return "ok";
    }
    console.error(e);
    toast(T.saveError, "error");
    return "config_failed";
  }

  toast(T.backup.importSuccess, "success");
  toast(T.backup.rebootHint, "info", 6000);
  await refreshDevice(signal);
  return "ok";
}

export async function applyBackupParts(
  parts: Record<(typeof PART_ORDER)[number], BalansunBackup>,
  signal?: AbortSignal,
  caps?: DeviceCapabilities,
): Promise<boolean> {
  const T = getStrings();
  const omitAuth =
    typeof location !== "undefined" && isAuthExemptPath(location.pathname || "/");
  const opts = { signal, retry: 0 as const, omitAuth };
  const targetCaps = caps ?? deviceInfo.get()?.capabilities;
  let step = 0;
  for (const name of PART_ORDER) {
    const partDoc = parts[name];
    if (!partDoc) continue;
    step++;
    try {
      await api.putSystemBackupPart(name, partDoc, opts);
      toast(T.backup.importPartProgress.replace("{n}", String(step)).replace("{total}", "3"), "info", 2500);
    } catch (e) {
      console.error(e);
      toast(T.backup.importPartFailed.replace("{part}", name), "error");
      return false;
    }
  }
  if (parts.config) {
    const adapted = adaptBackupForTarget(parts.config, targetCaps);
    const pinPatch = pinPatchFromBackupConfig(adapted.config);
    if (pinPatch) {
      try {
        await api.putSystemPins(pinPatch, opts);
      } catch (e) {
        console.error(e);
        toast(T.saveError, "error");
        return false;
      }
    }
  }
  toast(T.backup.importSuccess, "success");
  toast(T.backup.rebootHint, "info", 6000);
  await refreshDevice(signal);
  return true;
}

export async function confirmRestoreBackupFromFile(
  file: File,
  signal?: AbortSignal,
): Promise<void> {
  const T = getStrings();
  let text: string;
  try {
    text = await file.text();
  } catch {
    toast(T.backup.parseErrors.invalidJson, "error");
    return;
  }
  const parsed = parseBackupJson(text);
  if (!parsed.ok) {
    const msg =
      T.backup.parseErrors[parsed.errorKey as keyof typeof T.backup.parseErrors] ||
      T.unknown;
    toast(msg, "error");
    return;
  }
  const backup = parsed.backup;
  openDialog({
    title: T.backup.importConfirmTitle,
    body: h("p", {}, T.backup.importConfirmBody),
    actions: [
      { label: T.cancel, kind: "ghost", onClick: () => {} },
      {
        label: T.backup.importApply,
        kind: "danger",
        onClick: async () => {
          await applyBackup(backup, signal);
        },
      },
    ],
  });
}

export async function confirmRestoreBackupFromFiles(
  files: File[],
  signal?: AbortSignal,
): Promise<void> {
  const T = getStrings();
  const parts: Partial<Record<(typeof PART_ORDER)[number], BalansunBackup>> = {};
  for (const file of files) {
    let text: string;
    try {
      text = await file.text();
    } catch {
      toast(T.backup.parseErrors.invalidJson, "error");
      return;
    }
    let raw: unknown;
    try {
      raw = JSON.parse(text) as unknown;
    } catch {
      toast(T.backup.parseErrors.invalidJson, "error");
      return;
    }
    if (typeof raw !== "object" || raw === null || Array.isArray(raw)) {
      toast(T.backup.parseErrors.notObject, "error");
      return;
    }
    const part = detectBackupPartFromJson(raw as Record<string, unknown>);
    if (!part) {
      const monolithic = parseBackupJson(text);
      if (monolithic.ok && !monolithic.backup.part) {
        openDialog({
          title: T.backup.importConfirmTitle,
          body: h("p", {}, T.backup.importConfirmBody),
          actions: [
            { label: T.cancel, kind: "ghost", onClick: () => {} },
            {
              label: T.backup.importApply,
              kind: "danger",
              onClick: async () => {
                await applyBackup(monolithic.backup, signal);
              },
            },
          ],
        });
        return;
      }
      toast(T.backup.parseErrors.unknownPart, "error");
      return;
    }
    const parsed = parseBackupJson(text);
    if (!parsed.ok) {
      const msg =
        T.backup.parseErrors[parsed.errorKey as keyof typeof T.backup.parseErrors] ||
        T.unknown;
      toast(msg, "error");
      return;
    }
    parts[part] = parsed.backup;
  }
  if (!parts.network?.wifi) {
    toast(T.backup.parseErrors.missingWifi, "error");
    return;
  }
  openDialog({
    title: T.backup.importConfirmTitle,
    body: h("p", {}, T.backup.importPartsConfirmBody),
    actions: [
      { label: T.cancel, kind: "ghost", onClick: () => {} },
      {
        label: T.backup.importApply,
        kind: "danger",
        onClick: async () => {
          await applyBackupParts(parts as Record<(typeof PART_ORDER)[number], BalansunBackup>, signal);
        },
      },
    ],
  });
}

export { isSparseBackup };

async function refreshDevice(signal?: AbortSignal): Promise<void> {
  try {
    const d = await api.getDevice({ signal, retry: 1 });
    deviceInfo.set({
      router_name: d.router_name,
      firmware_version: d.firmware_version,
      probe_house_name: d.probe_house_name,
      probe_second_name: d.probe_second_name,
      temperature_label: d.temperature_label,
      capabilities: d.capabilities,
    });
  } catch {
    // Non-fatal.
  }
}
