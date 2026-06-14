import type { ActionsConfigEnvelope, RouterConfig } from "../api/types";
import { PERSISTED_CONFIG_KEYS, pickPersistedConfig } from "../api/configPut";
import { ensureNormalised } from "../routes/actions/model";
export { sanitizeConfigForImport } from "../api/configPut";

/** Match Actions page when device GET returns `actions: []` (NbActions == 0). */
export function normalizeActionsForBackup(
  env: ActionsConfigEnvelope,
): ActionsConfigEnvelope {
  const actions = ensureNormalised(env.actions ?? []);
  return {
    schema_version: env.schema_version,
    nb_actions: actions.length,
    actions,
  };
}

/** Client-side backup file format (not the firmware REST envelope). */
export const BACKUP_SCHEMA_VERSION = 1 as const;
/** Matches firmware kMaxRoutingActions / actions UI cap. */
export const BACKUP_MAX_ACTIONS = 20;

const BACKUP_TOP_LEVEL_KEYS = [
  "backupSchemaVersion",
  "exportedAt",
  "device",
  "config",
  "actions",
  "time",
  "wifi",
  "api",
  "part",
] as const;

export interface BalansunBackupDevice {
  tier?: string;
  export_mode?: "sparse" | "full" | "sparse_parts";
  defaults_profile?: string;
  full_config_export?: boolean;
  config_export_max_bytes?: number;
  put_body_max_bytes?: number;
  history_days_retained?: number;
  firmware_version?: string;
}

export interface BalansunBackupApiAccessToken {
  id: number;
  label: string;
  token: string;
}

export interface BalansunBackupApi {
  http_api_password?: string;
  access_tokens?: BalansunBackupApiAccessToken[];
}

export interface BalansunBackupTime {
  tz: string;
  ntp1: string;
  ntp2: string;
}

export interface BalansunBackupWifi {
  ssid: string;
  password: string;
}

export interface BalansunBackup {
  backupSchemaVersion: typeof BACKUP_SCHEMA_VERSION;
  exportedAt: string;
  device?: BalansunBackupDevice;
  part?: "network" | "config" | "actions";
  config?: RouterConfig;
  actions?: ActionsConfigEnvelope;
  time?: BalansunBackupTime;
  wifi?: BalansunBackupWifi;
  api?: BalansunBackupApi;
}

export interface BalansunBackupManifest {
  backupSchemaVersion: number;
  export_mode: "sparse_parts";
  exportedAt: string;
  parts: ("network" | "config" | "actions")[];
  device?: BalansunBackupDevice;
}

const TOKEN_HEX64 = /^[0-9a-f]{64}$/;

function parseBackupApi(
  raw: unknown,
): { ok: true; api: BalansunBackupApi } | { ok: false; errorKey: string } {
  if (!isPlainObject(raw)) {
    return { ok: false, errorKey: "badApi" };
  }
  const api: BalansunBackupApi = {};
  if ("http_api_password" in raw) {
    if (typeof raw["http_api_password"] !== "string") {
      return { ok: false, errorKey: "badApi" };
    }
    api.http_api_password = raw["http_api_password"];
  }
  if ("access_tokens" in raw) {
    const arr = raw["access_tokens"];
    if (!Array.isArray(arr)) {
      return { ok: false, errorKey: "badApiTokens" };
    }
    if (arr.length > 4) {
      return { ok: false, errorKey: "badApiTokens" };
    }
    const tokens: BalansunBackupApiAccessToken[] = [];
    for (const item of arr) {
      if (!isPlainObject(item)) {
        return { ok: false, errorKey: "badApiTokens" };
      }
      if (typeof item["id"] !== "number" || typeof item["token"] !== "string") {
        return { ok: false, errorKey: "badApiTokens" };
      }
      if (!TOKEN_HEX64.test(item["token"])) {
        return { ok: false, errorKey: "badApiTokens" };
      }
      tokens.push({
        id: item["id"],
        label: typeof item["label"] === "string" ? item["label"] : "",
        token: item["token"],
      });
    }
    api.access_tokens = tokens;
  }
  return { ok: true, api };
}

function isPlainObject(v: unknown): v is Record<string, unknown> {
  return typeof v === "object" && v !== null && !Array.isArray(v);
}

function isNonEmptyString(v: unknown): v is string {
  return typeof v === "string";
}

export function buildBackup(
  config: RouterConfig,
  actions: ActionsConfigEnvelope,
  time: BalansunBackupTime,
  wifi: BalansunBackupWifi,
  opts?: { fullConfigExport?: boolean },
): BalansunBackup {
  const actionsNorm = normalizeActionsForBackup(actions);
  const full = opts?.fullConfigExport !== false;
  return {
    backupSchemaVersion: BACKUP_SCHEMA_VERSION,
    exportedAt: new Date().toISOString(),
    config: pickPersistedConfig(config, { includePmqttBindings: full }),
    actions: {
      schema_version: actionsNorm.schema_version,
      nb_actions: actionsNorm.nb_actions,
      actions: actionsNorm.actions.map((a) => ({
        ...a,
        periods: a.periods.map((p) => ({ ...p })),
      })),
    },
    time: { ...time },
    wifi: { ...wifi },
  };
}

function migrateLegacyPmqttBindings(cfg: Record<string, unknown>): void {
  if (cfg["source"] !== "Pmqtt" || !Array.isArray(cfg["pmqtt_bindings"])) return;
  const topicDefault =
    typeof cfg["pmqtt_topic"] === "string" && cfg["pmqtt_topic"].length > 0
      ? cfg["pmqtt_topic"]
      : "";
  for (const item of cfg["pmqtt_bindings"]) {
    if (!isPlainObject(item)) continue;
    if (typeof item["topic"] !== "string" || item["topic"].length === 0) {
      item["topic"] = topicDefault;
    }
    if (typeof item["format"] !== "string" || item["format"].length === 0) {
      item["format"] = "json";
    }
    if (!("enabled" in item)) {
      item["enabled"] = true;
    }
  }
}

export function parseBackupJson(
  raw: string,
): { ok: true; backup: BalansunBackup } | { ok: false; errorKey: string } {
  let parsed: unknown;
  try {
    parsed = JSON.parse(raw) as unknown;
  } catch {
    return { ok: false, errorKey: "invalidJson" };
  }
  if (!isPlainObject(parsed)) {
    return { ok: false, errorKey: "notObject" };
  }
  const ver = parsed["backupSchemaVersion"];
  if (ver !== BACKUP_SCHEMA_VERSION) {
    return { ok: false, errorKey: "badSchemaVersion" };
  }
  for (const k of Object.keys(parsed)) {
    if (!(BACKUP_TOP_LEVEL_KEYS as readonly string[]).includes(k)) {
      return { ok: false, errorKey: "unknownTopLevelKey" };
    }
  }
  if (!isNonEmptyString(parsed["exportedAt"])) {
    return { ok: false, errorKey: "missingExportedAt" };
  }
  let deviceBlock: BalansunBackupDevice | undefined;
  if ("device" in parsed) {
    if (!isPlainObject(parsed["device"])) {
      return { ok: false, errorKey: "badDevice" };
    }
    deviceBlock = parsed["device"] as BalansunBackupDevice;
  }
  const partName = typeof parsed["part"] === "string" ? parsed["part"] : undefined;
  const sparseExport =
    deviceBlock?.export_mode === "sparse" ||
    deviceBlock?.export_mode === "sparse_parts" ||
    partName === "config";

  const cfg = parsed["config"];
  if (!sparseExport && !partName) {
    if (!isPlainObject(cfg)) {
      return { ok: false, errorKey: "missingConfig" };
    }
  }
  if (isPlainObject(cfg)) {
    migrateLegacyPmqttBindings(cfg);
    if (!sparseExport) {
      for (const k of PERSISTED_CONFIG_KEYS) {
        if (!(k in cfg)) {
          return { ok: false, errorKey: "incompleteConfig" };
        }
      }
      if (cfg["source"] === "Pmqtt") {
        if (!("pmqtt_bindings" in cfg)) {
          return { ok: false, errorKey: "missingPmqttBindings" };
        }
        if (!Array.isArray(cfg["pmqtt_bindings"])) {
          return { ok: false, errorKey: "badPmqttBindings" };
        }
      }
    }
  } else if (!sparseExport && partName !== "network" && partName !== "actions") {
    return { ok: false, errorKey: "missingConfig" };
  }

  const act = parsed["actions"];
  if (!sparseExport && !partName && !isPlainObject(act)) {
    return { ok: false, errorKey: "missingActions" };
  }
  if (!isPlainObject(act) && partName === "actions") {
    return { ok: false, errorKey: "missingActions" };
  }
  let actionsEnvelope: ActionsConfigEnvelope | undefined;
  if (isPlainObject(act)) {
    let arr = act["actions"];
    if (!Array.isArray(arr)) {
      return { ok: false, errorKey: "missingActions" };
    }
    let actionsList: unknown[] = arr;
    if (actionsList.length === 0 && !sparseExport) {
      actionsList = ensureNormalised([]) as unknown[];
    }
    if (actionsList.length > BACKUP_MAX_ACTIONS) {
      return { ok: false, errorKey: "actionsTooMany" };
    }
    if (typeof act["schema_version"] !== "number") {
      return { ok: false, errorKey: "actionsBadEnvelope" };
    }
    actionsEnvelope = {
      schema_version: act["schema_version"] as number,
      nb_actions: typeof act["nb_actions"] === "number" ? act["nb_actions"] : actionsList.length,
      actions: actionsList as ActionsConfigEnvelope["actions"],
    };
  }

  const timeObj = parsed["time"];
  let timeBlock: BalansunBackupTime | undefined;
  if (isPlainObject(timeObj)) {
    if (
      !isNonEmptyString(timeObj["tz"]) ||
      typeof timeObj["ntp1"] !== "string" ||
      typeof timeObj["ntp2"] !== "string"
    ) {
      return { ok: false, errorKey: "badTime" };
    }
    timeBlock = {
      tz: timeObj["tz"],
      ntp1: timeObj["ntp1"],
      ntp2: timeObj["ntp2"],
    };
  } else if (!sparseExport && !partName) {
    return { ok: false, errorKey: "missingTime" };
  }

  const wifiObj = parsed["wifi"];
  let wifiBlock: BalansunBackupWifi | undefined;
  if (isPlainObject(wifiObj)) {
    if (!isNonEmptyString(wifiObj["ssid"]) || typeof wifiObj["password"] !== "string") {
      return { ok: false, errorKey: "badWifi" };
    }
    wifiBlock = {
      ssid: wifiObj["ssid"],
      password: wifiObj["password"],
    };
  } else if (partName === "network" || (!sparseExport && !partName)) {
    return { ok: false, errorKey: "missingWifi" };
  }
  let apiBlock: BalansunBackupApi | undefined;
  if ("api" in parsed) {
    const apiParsed = parseBackupApi(parsed["api"]);
    if (!apiParsed.ok) {
      return { ok: false, errorKey: apiParsed.errorKey };
    }
    const hasPassword =
      apiParsed.api.http_api_password !== undefined &&
      apiParsed.api.http_api_password.length > 0;
    const hasTokens =
      apiParsed.api.access_tokens !== undefined && apiParsed.api.access_tokens.length > 0;
    if (hasPassword || hasTokens) {
      apiBlock = apiParsed.api;
    }
  }

  const backup: BalansunBackup = {
    backupSchemaVersion: BACKUP_SCHEMA_VERSION,
    exportedAt: parsed["exportedAt"],
    ...(deviceBlock !== undefined ? { device: deviceBlock } : {}),
    ...(partName === "network" || partName === "config" || partName === "actions"
      ? { part: partName }
      : {}),
    ...(isPlainObject(cfg) ? { config: cfg as unknown as RouterConfig } : {}),
    ...(actionsEnvelope !== undefined ? { actions: actionsEnvelope } : {}),
    ...(timeBlock !== undefined ? { time: timeBlock } : {}),
    ...(wifiBlock !== undefined ? { wifi: wifiBlock } : {}),
    ...(apiBlock !== undefined ? { api: apiBlock } : {}),
  };
  return { ok: true, backup };
}

export function backupPartFilename(part: "network" | "config" | "actions"): string {
  const d = new Date();
  const y = d.getFullYear();
  const m = String(d.getMonth() + 1).padStart(2, "0");
  const day = String(d.getDate()).padStart(2, "0");
  return `balansun-backup-${part}-${y}-${m}-${day}.json`;
}

export function detectBackupPartFromJson(parsed: Record<string, unknown>): BalansunBackup["part"] {
  if (parsed["part"] === "network" || parsed["part"] === "config" || parsed["part"] === "actions") {
    return parsed["part"];
  }
  if (parsed["wifi"] && !parsed["config"] && !parsed["actions"]) return "network";
  if (parsed["config"] && !parsed["wifi"] && !parsed["actions"]) return "config";
  if (parsed["actions"] && !parsed["wifi"] && !parsed["config"]) return "actions";
  return undefined;
}

export function downloadJsonFile(filename: string, data: unknown): void {
  const blob = new Blob([JSON.stringify(data, null, 2)], {
    type: "application/json",
  });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = filename;
  a.rel = "noopener";
  document.body.append(a);
  a.click();
  a.remove();
  URL.revokeObjectURL(url);
}

export function backupDownloadFilename(): string {
  const d = new Date();
  const y = d.getFullYear();
  const m = String(d.getMonth() + 1).padStart(2, "0");
  const day = String(d.getDate()).padStart(2, "0");
  return `balansun-${y}-${m}-${day}.json`;
}
