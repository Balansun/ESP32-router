import type { RouterConfig, SourceDiagnostics } from "../../api/types";
import type { AppStrings } from "../../i18n/locales/en";
import { formatStr } from "../../i18n/format";
import { fmtVolts } from "../../utils/format";
import { registryEntry, type SourceWireId } from "./sourceRegistry";

export interface SourceSummaryRow {
  label: string;
  value: string;
  status?: "ok" | "warn" | "err";
}

function normalizeWireId(raw: string): string {
  return String(raw || "")
    .trim()
    .toLowerCase()
    .replace(/\s+/g, "")
    .replace(/-/g, "");
}

export function sourceKeyForSummary(configured: string, cfgSource: string | undefined): string {
  const raw = configured.length > 0 ? configured : (cfgSource ?? "NotDef");
  return normalizeWireId(raw);
}

export function sourceFriendlyTitle(wireId: string, T: AppStrings): string {
  const key = normalizeWireId(wireId);
  const titles = T.settings.sourceSummary.titles;
  switch (key) {
    case "analog":
      return titles.analog;
    case "linky":
      return titles.linky;
    case "jsymk194":
      return titles.jsyMk194;
    case "enphase":
      return titles.enphase;
    case "smartg":
      return titles.smartg;
    case "shellyem":
      return titles.shellyem;
    case "balansunpeer":
      return titles.balansunPeer;
    case "jsymk333":
      return titles.jsyMk333;
    case "shellypro":
      return titles.shellypro;
    case "homew":
      return titles.homew;
    case "pmqtt":
      return titles.pmqtt;
    case "notdef":
      return titles.notdef;
    default:
      return wireId.trim() || titles.notdef;
  }
}

function formatWireLine(wireId: string, T: AppStrings): string {
  const id = wireId.trim() || "NotDef";
  return `${id} — ${sourceFriendlyTitle(id, T)}`;
}

function formatPeerIp(ip: string | undefined): string {
  const raw = (ip || "").trim();
  if (!raw || raw === "0.0.0.0") return "—";
  return raw;
}

function extProtocolLabel(_proto: string | undefined, T: AppStrings): string {
  return T.sourceWizard.extProtocolJson;
}

function connectionStatus(
  d: SourceDiagnostics | null,
  T: AppStrings,
): { text: string; status: "ok" | "warn" | "err" } {
  const SS = T.settings.sourceSummary;
  const diag = d?.diagnostics;
  const pollOk =
    diag?.last_poll_ok ??
    (d?.ext != null ? d.ext.last_poll_ok : undefined);
  const ok = !!(pollOk && d?.date_valid);
  const ms =
    diag?.last_poll_ms_ago ??
    d?.ext?.last_poll_ms_ago;
  const pollSuffix =
    ms != null && ms >= 0
      ? ` · ${formatStr(SS.lastPoll, { ms: String(ms) })}`
      : "";
  if (ok) {
    return { text: SS.statusOk + pollSuffix, status: "ok" };
  }
  const err =
    (diag?.last_error && diag.last_error.length > 0
      ? diag.last_error
      : undefined) ||
    (d?.ext?.last_error && d.ext.last_error.length > 0
      ? d.ext.last_error
      : undefined);
  if (err) {
    return { text: `${SS.statusError}: ${err}${pollSuffix}`, status: "err" };
  }
  return { text: SS.statusStale + pollSuffix, status: "warn" };
}

function peerUrlFromConfig(cfg: RouterConfig): string {
  const ip = formatPeerIp(cfg.ext_peer_ip);
  if (ip === "—") return "—";
  const port = cfg.ext_peer_port || 80;
  const path = (cfg.ext_peer_path || "").trim() || "/";
  const p = path.startsWith("/") ? path : `/${path}`;
  return `${ip}:${port}${p}`;
}

function peerUrlFromExt(d: SourceDiagnostics): string {
  const ext = d.ext!;
  const ip = formatPeerIp(ext.ext_peer_ip);
  if (ip === "—") return "—";
  let port = 80;
  if (ext.ext_peer_port) port = ext.ext_peer_port;
  const pathTrim = (ext.ext_peer_path ?? "").trim();
  let p = "/";
  if (pathTrim.length > 0) {
    p = pathTrim.startsWith("/") ? pathTrim : `/${pathTrim}`;
  }
  return `${ip}:${port}${p}`;
}

function asWireId(source: string): SourceWireId | undefined {
  const entry = registryEntry(source as SourceWireId);
  return entry?.id;
}

export function buildSourceSummaryRows(
  cfg: RouterConfig,
  d: SourceDiagnostics | null,
  T: AppStrings,
): SourceSummaryRow[] {
  const SS = T.settings.sourceSummary;
  const rows: SourceSummaryRow[] = [];
  const configured =
    d?.diagnostics?.active_source || d?.source || cfg.source || "NotDef";
  rows.push({
    label: SS.configured,
    value: formatWireLine(configured, T),
  });

  let effective: string | null = null;
  const meter = d?.diagnostics?.meter;
  const activeForMeter = d?.diagnostics?.active_source || configured;
  if (typeof meter === "string" && meter.length > 0 && meter !== activeForMeter) {
    effective = meter;
  }
  if (effective) {
    rows.push({
      label: SS.effectiveMeter,
      value: formatWireLine(effective, T),
    });
  }

  if (d) {
    const conn = connectionStatus(d, T);
    rows.push({
      label: SS.connection,
      value: conn.text,
      status: conn.status,
    });
    if (d.date_valid && d.date) {
      rows.push({ label: SS.lastSample, value: d.date });
    }
  }

  const wireFromCfg = asWireId(cfg.source);
  const wirePrimary = asWireId(configured);
  const wire = wirePrimary !== undefined ? wirePrimary : wireFromCfg;
  const sourceKey = sourceKeyForSummary(configured, cfg.source);

  if (sourceKey === "balansunpeer" || wire === "BalansunPeer") {
    const peer = d?.ext ? peerUrlFromExt(d) : peerUrlFromConfig(cfg);
    rows.push({ label: SS.peerUrl, value: peer });
    const protoCfg =
      d?.diagnostics?.ext_protocol ||
      d?.ext?.ext_protocol ||
      cfg.ext_protocol;
    rows.push({
      label: SS.protocolConfigured,
      value: extProtocolLabel(protoCfg, T),
    });
    const used =
      d?.diagnostics?.protocol_used || d?.ext?.protocol_used;
    if (used) {
      rows.push({
        label: SS.protocolUsed,
        value: extProtocolLabel(used, T),
      });
    }
    const sd =
      d?.diagnostics?.source_data ||
      d?.source_data ||
      "";
    if (sd && sd !== configured) {
      rows.push({
        label: SS.sourceData,
        value: formatWireLine(sd, T),
      });
    }
  }

  const lanSources = new Set([
    "enphase",
    "smartg",
    "shellyem",
    "homew",
    "shellypro",
  ]);
  if (lanSources.has(sourceKey)) {
    const ip =
      formatPeerIp(cfg.ext_peer_ip) !== "—"
        ? formatPeerIp(cfg.ext_peer_ip)
        : formatPeerIp(d?.ext?.ext_peer_ip);
    rows.push({ label: SS.targetIp, value: ip });
  }

  if (sourceKey === "enphase" && d?.enphase) {
    if (d.enphase.user) {
      rows.push({ label: SS.enphaseUser, value: d.enphase.user });
    }
    if (d.enphase.serial) {
      rows.push({ label: SS.enphaseSerial, value: d.enphase.serial });
    }
  }

  if (sourceKey === "pmqtt") {
    if (cfg.pmqtt_topic) {
      rows.push({ label: SS.pmqttTopic, value: cfg.pmqtt_topic });
    }
    if (cfg.pmqtt_schema) {
      rows.push({ label: SS.pmqttSchema, value: cfg.pmqtt_schema });
    }
  }

  if (sourceKey === "jsymk333" && cfg.jsy_mk333_serial_baud) {
    rows.push({
      label: SS.jsyMk333Baud,
      value: String(cfg.jsy_mk333_serial_baud),
    });
  }

  if (sourceKey === "analog") {
    rows.push(
      { label: SS.calibU, value: String(cfg.calib_u ?? 1000) },
      { label: SS.calibI, value: String(cfg.calib_i ?? 1000) },
    );
  }

  if (sourceKey === "linky" && d?.linky) {
    if (d.linky.ltarf) {
      rows.push({ label: SS.linkyTariff, value: d.linky.ltarf });
    }
    if (d.linky.cacsi_no_export) {
      rows.push({
        label: SS.cacsiLabel,
        value: SS.cacsiWarn,
        status: "warn",
      });
    }
  }

  const physicalMeter = new Set(["jsymk194", "analog", "linky", "jsymk333"]);
  if (physicalMeter.has(sourceKey) && d) {
    if (Number.isFinite(d.voltage_house_v) && d.voltage_house_v > 0) {
      rows.push({
        label: SS.voltage,
        value: `${fmtVolts(d.voltage_house_v)} V`,
      });
    }
    const hz = d.frequency_hz || d.diagnostics?.mains_frequency_hz;
    if (hz != null && Number.isFinite(hz) && hz > 0) {
      rows.push({
        label: SS.frequency,
        value: `${hz.toFixed(2)} Hz`,
      });
    }
  }

  return rows;
}
