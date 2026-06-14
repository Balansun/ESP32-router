import type {
  ActionsConfigEnvelope,
  ActionsLive,
  ActionOverride,
  ApiOk,
  MqttTestResponse,
  PmqttBinding,
  PmqttPreviewResponse,
  ArduinoOtaInfo,
  ArduinoOtaPutResponse,
  ConfigEnvelope,
  DeviceInfo,
  FirmwareOtaResponse,
  HttpAuthPutResponse,
  AuthTokenInfo,
  AuthTokenCreateResponse,
  PublicInfo,
  HealthInfo,
  HardwarePinMap,
  PinMapPutResponse,
  HistoryEnergyDaily,
  HistoryPower,
  OverrideRequest,
  Measurements,
  TempoTariffStatus,
  RouterConfig,
  SourceDiagnostics,
  SourcesInfo,
  StateEnvelope,
  SystemInfo,
  TimeInfo,
  WifiInfo,
  WifiScanResult,
} from "./types";
import { isAuthExemptPath, requestHttpAuthLogin } from "../auth/httpAuthGate";
import { getSessionAuthHeader } from "./apiSession";
import { ApiNetworkError, isBrowserNetworkFailure } from "./networkFailure";
import { scheduleApi } from "./requestScheduler";
import { ensureRouterConfigPutPayload } from "./configPut";

export class ApiError extends Error {
  status: number;
  body: unknown;
  constructor(message: string, status: number, body: unknown) {
    super(message);
    this.status = status;
    this.body = body;
  }
}

interface FetchOpts {
  signal?: AbortSignal;
  retry?: number;
  retryDelayMs?: number;
  timeoutMs?: number;
  /** Do not send stored session token (first password set on open LAN). */
  omitAuth?: boolean;
  /** Internal: skip 401 re-login retry (after one attempt). */
  _authRetried?: boolean;
}

function mergeAuthHeaders(headers: HeadersInit | undefined): HeadersInit {
  const auth = getSessionAuthHeader();
  if (!auth) return headers ?? {};
  if (headers instanceof Headers) {
    const h = new Headers(headers);
    if (!h.has("Authorization")) h.set("Authorization", auth);
    return h;
  }
  const base =
    headers && typeof headers === "object" && !Array.isArray(headers)
      ? { ...headers }
      : {};
  if (!("Authorization" in base) && !("authorization" in base)) {
    (base as Record<string, string>).Authorization = auth;
  }
  return base;
}

export interface PollWifiScanOpts extends FetchOpts {
  /** Max wall time waiting for `scanning: false` (default 15000). */
  maxWaitMs?: number;
  /** Delay between polls when scan in progress (default 400). */
  pollIntervalMs?: number;
}

export interface HistoryEnergyDailyQuery {
  limit?: number;
  offset?: number;
  fromDateIso?: string;
  toDateIso?: string;
}

async function jsonFetch<T>(
  url: string,
  init: RequestInit,
  opts: FetchOpts = {},
): Promise<T> {
  const retry = opts.retry ?? 0;
  const retryDelayMs = opts.retryDelayMs ?? 600;
  const timeoutMs = opts.timeoutMs ?? 8000;
  let lastErr: unknown;
  for (let attempt = 0; attempt <= retry; attempt++) {
    try {
      return await scheduleApi(async () => {
        const ctrl = new AbortController();
        const timer = setTimeout(() => ctrl.abort(), timeoutMs);
        if (opts.signal) {
          if (opts.signal.aborted) ctrl.abort();
          else
            opts.signal.addEventListener("abort", () => ctrl.abort(), {
              once: true,
            });
        }
        try {
          const headers = opts.omitAuth
            ? init.headers ?? { Accept: "application/json" }
            : mergeAuthHeaders(init.headers);
          const res = await fetch(url, {
            ...init,
            headers,
            signal: ctrl.signal,
          });
          clearTimeout(timer);
          const ct = res.headers.get("Content-Type") || "";
          const isJson = ct.indexOf("application/json") >= 0;
          const body = isJson ? await res.json() : await res.text();
          if (res.status === 401 && !opts._authRetried) {
            const onWifiPage =
              typeof location !== "undefined" &&
              isAuthExemptPath(location.pathname || "/");
            if (!onWifiPage) {
              const ok = await requestHttpAuthLogin();
              if (ok) {
                return jsonFetch<T>(url, init, { ...opts, _authRetried: true });
              }
            }
          }
          if (!res.ok) {
            throw new ApiError(
              `HTTP ${res.status} ${res.statusText}`,
              res.status,
              body,
            );
          }
          return body as T;
        } catch (e) {
          clearTimeout(timer);
          if (isBrowserNetworkFailure(e)) {
            throw new ApiNetworkError("device");
          }
          throw e;
        }
      });
    } catch (e) {
      lastErr = e;
      if (opts.signal?.aborted) throw e;
      if (attempt < retry) {
        await new Promise((r) =>
          setTimeout(r, retryDelayMs * Math.pow(2, attempt)),
        );
        continue;
      }
      throw e;
    }
  }
  throw lastErr;
}

const get = <T>(url: string, opts: FetchOpts = {}) =>
  jsonFetch<T>(url, { method: "GET", headers: { Accept: "application/json" } }, opts);

function delay(ms: number, signal?: AbortSignal): Promise<void> {
  return new Promise((resolve, reject) => {
    if (signal?.aborted) {
      reject(new DOMException("Aborted", "AbortError"));
      return;
    }
    const id = setTimeout(() => {
      signal?.removeEventListener("abort", onAbort);
      resolve();
    }, ms);
    const onAbort = () => {
      clearTimeout(id);
      reject(new DOMException("Aborted", "AbortError"));
    };
    signal?.addEventListener("abort", onAbort, { once: true });
  });
}

const send = <T>(method: string, url: string, body: unknown, opts: FetchOpts = {}) =>
  jsonFetch<T>(
    url,
    {
      method,
      headers: { "Content-Type": "application/json", Accept: "application/json" },
      body: JSON.stringify(body ?? {}),
    },
    opts,
  );

function isWifiEmptyInputError(err: unknown): boolean {
  if (!(err instanceof ApiError) || err.status !== 400) return false;
  if (!err.body || typeof err.body !== "object") return false;
  const body = err.body as { error?: unknown; message?: unknown };
  return body.error === "json_parse" && body.message === "EmptyInput";
}

export interface PostFirmwareOtaOpts {
  signal?: AbortSignal;
  /** Default 180000 (3 minutes). */
  timeoutMs?: number;
  /** 32 hex chars; sent as query `md5=` for ESP32 `Update.setMD5`. */
  md5?: string;
}

async function postFirmwareOta(
  file: File,
  opts: PostFirmwareOtaOpts = {},
): Promise<FirmwareOtaResponse> {
  const timeoutMs = opts.timeoutMs ?? 180_000;
  const ctrl = new AbortController();
  const timer = setTimeout(() => ctrl.abort(), timeoutMs);
  if (opts.signal) {
    if (opts.signal.aborted) ctrl.abort();
    else opts.signal.addEventListener("abort", () => ctrl.abort(), { once: true });
  }
  const fd = new FormData();
  fd.append("firmware", file, file.name);
  let url = "/api/v1/firmware/ota";
  const md5 = opts.md5?.trim();
  if (md5 && /^[0-9a-fA-F]{32}$/.test(md5)) {
    url += `?md5=${encodeURIComponent(md5.toLowerCase())}`;
  }
  try {
    const headers = mergeAuthHeaders({ Accept: "application/json" });
    const res = await fetch(url, {
      method: "POST",
      body: fd,
      signal: ctrl.signal,
      headers,
    });
    clearTimeout(timer);
    const ct = res.headers.get("Content-Type") || "";
    const isJson = ct.indexOf("application/json") >= 0;
    const body = isJson ? await res.json() : await res.text();
    if (res.status === 401) {
      const ok = await requestHttpAuthLogin();
      if (ok) return postFirmwareOta(file, opts);
    }
    if (!res.ok) {
      throw new ApiError(
        `HTTP ${res.status} ${res.statusText}`,
        res.status,
        body,
      );
    }
    return body as FirmwareOtaResponse;
  } catch (e) {
    clearTimeout(timer);
    if (isBrowserNetworkFailure(e)) {
      throw new ApiNetworkError("device");
    }
    throw e;
  }
}

export const api = {
  getMeasurements: (opts?: FetchOpts) =>
    get<Measurements>("/api/v1/measurements", opts),
  getTariffTempo: (opts?: FetchOpts) =>
    get<TempoTariffStatus>("/api/v1/tariff/tempo", opts),
  getSystem: (opts?: FetchOpts) => get<SystemInfo>("/api/v1/system", opts),
  getDevice: (opts?: FetchOpts) => get<DeviceInfo>("/api/v1/device", opts),
  getState: (opts?: FetchOpts) => get<StateEnvelope>("/api/v1/state", opts),
  getHealth: (opts?: FetchOpts) => get<HealthInfo>("/api/v1/health", opts),
  getSystemAudit: (opts?: FetchOpts) =>
    get<import("./types").ConfigAuditEnvelope>("/api/v1/system/audit", opts),
  postHealthSelfTestRun: (opts?: FetchOpts) =>
    send<ApiOk>("POST", "/api/v1/health/self-test/run", {}, opts),
  postHealthSelfTestSkip: (opts?: FetchOpts) =>
    send<ApiOk>("POST", "/api/v1/health/self-test/skip", {}, opts),
  postHealthHardwareRecheck: (id: string, opts?: FetchOpts) =>
    send<{ ok: boolean; item: import("./types").HardwarePresenceItem }>(
      "POST",
      "/api/v1/health/hardware/recheck",
      { id },
      opts,
    ),
  postStatusLedTest: (body: import("./types").StatusLedTestRequest, opts?: FetchOpts) =>
    send<import("./types").StatusLedTestResponse>(
      "POST",
      "/api/v1/hardware/status-led/test",
      body,
      opts,
    ),
  getPublic: (opts?: FetchOpts) =>
    get<PublicInfo>("/api/v1/public", { ...opts, omitAuth: true }),
  getSources: (opts?: FetchOpts) => get<SourcesInfo>("/api/v1/sources", opts),

  getConfig: (opts?: FetchOpts) => get<ConfigEnvelope>("/api/v1/config", opts),

  getSystemPins: (opts?: FetchOpts) => get<HardwarePinMap>("/api/v1/system/pins", opts),

  putSystemPins: (body: Partial<HardwarePinMap>, opts?: FetchOpts) =>
    send<PinMapPutResponse>("PUT", "/api/v1/system/pins", body, opts),
  putConfig: (cfg: RouterConfig, opts?: FetchOpts) =>
    send<ApiOk>(
      "PUT",
      "/api/v1/config",
      { config: ensureRouterConfigPutPayload(cfg) },
      opts,
    ),
  patchConfig: (partial: Partial<RouterConfig>, opts?: FetchOpts) =>
    send<ApiOk>("PATCH", "/api/v1/config", partial, opts),

  getActionsLive: (opts?: FetchOpts) => get<ActionsLive>("/api/v1/actions", opts),
  getActionsConfig: (opts?: FetchOpts) =>
    get<ActionsConfigEnvelope>("/api/v1/actions/config", opts),
  putActionsConfig: (env: ActionsConfigEnvelope, opts?: FetchOpts) =>
    send<ApiOk>("PUT", "/api/v1/actions/config", env, opts),
  getActionOverride: (index: number, opts?: FetchOpts) =>
    get<ActionOverride>(`/api/v1/actions/${index}/override`, opts),
  postActionOverride: (index: number, override: OverrideRequest, opts?: FetchOpts) =>
    send<ApiOk>("POST", `/api/v1/actions/${index}/override`, override, opts),
  clearActionOverride: (index: number, opts?: FetchOpts) =>
    send<ApiOk>("POST", `/api/v1/actions/${index}/override/clear`, {}, opts),

  getHistoryPower: (
    window: "10m" | "24h" | "48h",
    maxPoints = 300,
    opts?: FetchOpts,
  ) =>
    get<HistoryPower>(
      `/api/v1/history/power?window=${window}&max_points=${maxPoints}`,
      opts,
    ),
  getHistoryEnergyDaily: (opts?: FetchOpts) =>
    get<HistoryEnergyDaily>("/api/v1/history/energy/daily", opts),
  getHistoryEnergyDailyPage: (query: HistoryEnergyDailyQuery = {}, opts?: FetchOpts) => {
    const p = new URLSearchParams();
    if (typeof query.limit === "number") p.set("limit", String(query.limit));
    if (typeof query.offset === "number") p.set("offset", String(query.offset));
    if (query.fromDateIso) p.set("from_date", query.fromDateIso);
    if (query.toDateIso) p.set("to_date", query.toDateIso);
    const q = p.toString();
    const url = q
      ? `/api/v1/history/energy/daily?${q}`
      : "/api/v1/history/energy/daily";
    return get<HistoryEnergyDaily>(url, opts);
  },
  getHistoryEnergyDailyRangeAll: async (
    fromDateIso: string,
    toDateIso: string,
    opts?: FetchOpts,
  ): Promise<HistoryEnergyDaily> => {
    const pageLimit = 10;
    let offset = 0;
    const merged: HistoryEnergyDaily = {
      delta_wh_per_day: [],
      count: 0,
      idx_prom_du_jour: 0,
      day_dates_iso: [],
      import_wh_per_day: [],
      export_wh_per_day: [],
      ch1_import_wh_per_day: [],
      ch1_export_wh_per_day: [],
      ch2_import_wh_per_day: [],
      ch2_export_wh_per_day: [],
      items: [],
    };
    for (;;) {
      const page = await api.getHistoryEnergyDailyPage(
        { fromDateIso, toDateIso, limit: pageLimit, offset },
        opts,
      );
      if (offset === 0) {
        merged.idx_prom_du_jour = page.idx_prom_du_jour;
        merged.reference_date_iso = page.reference_date_iso;
        merged.days_capacity = page.days_capacity;
        merged.days_retained = page.days_retained;
      }
      merged.delta_wh_per_day.push(...(page.delta_wh_per_day || []));
      merged.day_dates_iso?.push(...(page.day_dates_iso || []));
      merged.import_wh_per_day?.push(...(page.import_wh_per_day || []));
      merged.export_wh_per_day?.push(...(page.export_wh_per_day || []));
      merged.ch1_import_wh_per_day?.push(...(page.ch1_import_wh_per_day || []));
      merged.ch1_export_wh_per_day?.push(...(page.ch1_export_wh_per_day || []));
      merged.ch2_import_wh_per_day?.push(...(page.ch2_import_wh_per_day || []));
      merged.ch2_export_wh_per_day?.push(...(page.ch2_export_wh_per_day || []));
      merged.items?.push(...(page.items || []));
      const pageCount = page.count || (page.delta_wh_per_day?.length ?? 0);
      offset += pageCount;
      if (!page.has_more || pageCount <= 0) break;
    }
    merged.count = merged.delta_wh_per_day.length;
    merged.total_count = merged.count;
    merged.limit = pageLimit;
    merged.offset = 0;
    merged.has_more = false;
    return merged;
  },

  reboot: (opts?: FetchOpts) =>
    send<ApiOk>("POST", "/api/v1/system/reboot", {}, opts),

  resetPins: (opts?: FetchOpts) =>
    send<ApiOk & { reboot_required?: boolean }>("POST", "/api/v1/system/pins/reset", {}, opts),

  getWifi: (opts?: FetchOpts) => get<WifiInfo>("/api/v1/wifi", opts),
  putWifi: async (
    body: { ssid: string; password: string; persist?: boolean },
    opts?: FetchOpts,
  ) => {
    try {
      return await send<ApiOk>("PUT", "/api/v1/wifi", body, opts);
    } catch (e) {
      if (!isWifiEmptyInputError(e)) throw e;
      const qp = new URLSearchParams({
        ssid: body.ssid,
        password: body.password ?? "",
        persist: body.persist === false ? "0" : "1",
      });
      // Some captive portal paths/proxies strip JSON bodies on PUT.
      return send<ApiOk>("PUT", `/api/v1/wifi?${qp.toString()}`, {}, opts);
    }
  },
  /** Single GET; for in-progress scans the firmware may return HTTP 202 (still `ok` in fetch). */
  scanWifi: (opts?: FetchOpts) => get<WifiScanResult>("/api/v1/wifi/scan", opts),
  /**
   * Polls GET /api/v1/wifi/scan until `scanning` is false or `maxWaitMs` elapses.
   * Works with HTTP 200/202 while the radio scan is running.
   */
  pollWifiScan: async (opts?: PollWifiScanOpts): Promise<WifiScanResult> => {
    const maxWait = opts?.maxWaitMs ?? 15_000;
    const interval = opts?.pollIntervalMs ?? 400;
    const t0 = Date.now();
    const fetchOpts: FetchOpts = {
      signal: opts?.signal,
      timeoutMs: opts?.timeoutMs ?? 8000,
      retry: opts?.retry,
      retryDelayMs: opts?.retryDelayMs,
    };
    for (;;) {
      const data = await jsonFetch<WifiScanResult>(
        "/api/v1/wifi/scan",
        {
          method: "GET",
          headers: mergeAuthHeaders({ Accept: "application/json" }),
        },
        fetchOpts,
      );
      if (!data.scanning) return data;
      if (Date.now() - t0 > maxWait) {
        throw new ApiError("Wi‑Fi scan timed out", 408, data);
      }
      await delay(interval, opts?.signal);
    }
  },
  factoryReset: (opts?: FetchOpts) =>
    send<ApiOk>("POST", "/api/v1/system/factory-reset", {}, opts),
  saveNow: (opts?: FetchOpts) =>
    send<ApiOk>("POST", "/api/v1/system/save-now", {}, opts),
  getSystemBackup: (opts?: FetchOpts) =>
    get<import("../utils/backupFormat").BalansunBackup>("/api/v1/system/backup", opts),
  putSystemBackup: (body: import("../utils/backupFormat").BalansunBackup, opts?: FetchOpts) =>
    send<ApiOk>("PUT", "/api/v1/system/backup", body, opts),
  getSystemBackupManifest: (opts?: FetchOpts) =>
    get<import("../utils/backupFormat").BalansunBackupManifest>(
      "/api/v1/system/backup/manifest",
      opts,
    ),
  getSystemBackupPart: (part: "network" | "config" | "actions", opts?: FetchOpts) =>
    get<import("../utils/backupFormat").BalansunBackup>(
      `/api/v1/system/backup/part/${part}`,
      opts,
    ),
  putSystemBackupPart: (
    part: "network" | "config" | "actions",
    body: import("../utils/backupFormat").BalansunBackup,
    opts?: FetchOpts,
  ) => send<ApiOk>("PUT", `/api/v1/system/backup/part/${part}`, body, opts),
  getEeprom: (opts?: FetchOpts) => get<Record<string, unknown>>("/api/v1/system/eeprom", opts),
  getArduinoOta: (opts?: FetchOpts) => get<ArduinoOtaInfo>("/api/v1/system/arduino-ota", opts),
  putArduinoOtaPassword: (password: string, opts?: FetchOpts) =>
    send<ArduinoOtaPutResponse>("PUT", "/api/v1/system/arduino-ota", { password }, opts),
  putHttpAuthPassword: (password: string, opts?: FetchOpts) =>
    send<HttpAuthPutResponse>("PUT", "/api/v1/system/http-auth", { password }, opts),
  listAuthTokens: (opts?: FetchOpts) => get<AuthTokenInfo[]>("/api/v1/auth/tokens", opts),
  createAuthToken: (label?: string, opts?: FetchOpts) =>
    send<AuthTokenCreateResponse>(
      "POST",
      "/api/v1/auth/tokens",
      label ? { label } : {},
      opts,
    ),
  revokeAuthToken: (id: number, opts?: FetchOpts) =>
    send<ApiOk>("DELETE", `/api/v1/auth/tokens/${id}`, {}, opts),
  /** Set or clear HTTP API password; omits session token when LAN is still open. */
  async putHttpAuthPasswordOpen(
    password: string,
    opts?: FetchOpts,
  ): Promise<HttpAuthPutResponse> {
    let omitAuth = true;
    try {
      const pub = await get<PublicInfo>("/api/v1/public", opts);
      omitAuth = !pub.http_auth.enabled;
    } catch {
      /* device unreachable — try without stale credentials */
    }
    return send<HttpAuthPutResponse>(
      "PUT",
      "/api/v1/system/http-auth",
      { password },
      { ...opts, omitAuth },
    );
  },
  getTime: (opts?: FetchOpts) => get<TimeInfo>("/api/v1/time", opts),
  putTime: (body: Partial<Pick<TimeInfo, "tz" | "ntp1" | "ntp2">>, opts?: FetchOpts) =>
    send<ApiOk>("PUT", "/api/v1/time", body, opts),

  getSourceDiagnostics: (linkyTail?: number, opts?: FetchOpts) =>
    get<SourceDiagnostics>(
      linkyTail
        ? `/api/v1/sources/diagnostics?linky_tail=${linkyTail}`
        : "/api/v1/sources/diagnostics",
      opts,
    ),
  resetHistory: (opts?: FetchOpts) =>
    send<ApiOk>("POST", "/api/v1/history/reset", {}, opts),
  mqttRepublishDiscovery: (opts?: FetchOpts) =>
    send<ApiOk>("POST", "/api/v1/mqtt/discover", {}, opts),
  mqttReconnect: (opts?: FetchOpts) =>
    send<ApiOk>("POST", "/api/v1/mqtt/reconnect", {}, opts),
  mqttPublishNow: (opts?: FetchOpts) =>
    send<ApiOk>("POST", "/api/v1/mqtt/publish-now", {}, opts),
  mqttTest: (
    body: {
      mqtt_ip: string;
      mqtt_port: number;
      mqtt_user?: string;
      mqtt_password?: string;
      mqtt_device_name?: string;
    },
    opts?: FetchOpts,
  ) => send<MqttTestResponse>("POST", "/api/v1/mqtt/test", body, opts),
  pmqttPreview: (bindings: PmqttBinding[], opts?: FetchOpts) =>
    send<PmqttPreviewResponse>("POST", "/api/v1/sources/pmqtt/preview", { pmqtt_bindings: bindings }, opts),
  /** Lab / `env:hil` firmware only; returns 404 if disabled. */
  postSourceTestInject: (body: Partial<Measurements>, opts?: FetchOpts) =>
    send<ApiOk>("POST", "/api/v1/sources/test/inject", body, opts),

  /** Multipart upload of a `.bin` firmware image; device reboots on success. */
  postFirmwareOta: (file: File, opts?: PostFirmwareOtaOpts) =>
    postFirmwareOta(file, opts),

};

export type Api = typeof api;
