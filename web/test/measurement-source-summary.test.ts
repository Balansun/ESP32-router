import { describe, expect, it } from "vitest";
import { en } from "../src/i18n/locales/en";
import type { RouterConfig, SourceDiagnostics } from "../src/api/types";
import {
  buildSourceSummaryRows,
  sourceFriendlyTitle,
} from "../src/routes/settings/measurementSourceSummary";

describe("sourceFriendlyTitle", () => {
  it("normalizes wire ids", () => {
    expect(sourceFriendlyTitle("linky", en)).toBe(en.settings.sourceSummary.titles.linky);
    expect(sourceFriendlyTitle("Shelly-Em", en)).toBe(
      en.settings.sourceSummary.titles.shellyem,
    );
    expect(sourceFriendlyTitle("unknown", en)).toBe("unknown");
  });
});

describe("buildSourceSummaryRows", () => {
  const baseCfg: RouterConfig = {
    source: "BalansunPeer",
    ext_peer_ip: "192.168.1.50",
    ext_peer_port: 8080,
    ext_peer_path: "/api/v1/measurements",
    ext_protocol: "json",
  } as RouterConfig;

  it("includes ext peer URL and protocol rows", () => {
    const rows = buildSourceSummaryRows(baseCfg, null, en);
    const labels = rows.map((r) => r.label);
    expect(labels).toContain(en.settings.sourceSummary.configured);
    expect(labels).toContain(en.settings.sourceSummary.peerUrl);
    const peer = rows.find((r) => r.label === en.settings.sourceSummary.peerUrl);
    expect(peer?.value).toBe("192.168.1.50:8080/api/v1/measurements");
  });

  it("reports connection ok when poll and date valid", () => {
    const d: SourceDiagnostics = {
      source: "BalansunPeer",
      date_valid: true,
      date: "2026-05-20T10:00:00Z",
      diagnostics: {
        last_poll_ok: true,
        last_poll_ms_ago: 1200,
      },
    } as SourceDiagnostics;
    const rows = buildSourceSummaryRows(baseCfg, d, en);
    const conn = rows.find((r) => r.label === en.settings.sourceSummary.connection);
    expect(conn?.status).toBe("ok");
    expect(conn?.value).toContain(en.settings.sourceSummary.statusOk);
  });

  it("warns on linky CACSI no export", () => {
    const cfg = { source: "Linky" } as RouterConfig;
    const d: SourceDiagnostics = {
      source: "Linky",
      linky: { ltarf: "HPHC", cacsi_no_export: true },
    } as SourceDiagnostics;
    const rows = buildSourceSummaryRows(cfg, d, en);
    const cacsi = rows.find((r) => r.label === en.settings.sourceSummary.cacsiLabel);
    expect(cacsi?.status).toBe("warn");
  });

  it("includes pmqtt topic when configured", () => {
    const cfg = {
      source: "Pmqtt",
      pmqtt_topic: "home/power",
      pmqtt_schema: "pw",
    } as RouterConfig;
    const rows = buildSourceSummaryRows(cfg, null, en);
    expect(rows.some((r) => r.value === "home/power")).toBe(true);
    expect(rows.some((r) => r.label === en.settings.sourceSummary.pmqttSchema)).toBe(true);
  });

  it("adds peer URL with leading slash when path omits it", () => {
    const cfg = {
      source: "BalansunPeer",
      ext_peer_ip: "10.0.0.1",
      ext_peer_port: 80,
      ext_peer_path: "api/x",
    } as RouterConfig;
    const peer = buildSourceSummaryRows(cfg, null, en).find(
      (r) => r.label === en.settings.sourceSummary.peerUrl,
    );
    expect(peer?.value).toContain("/api/x");
  });

  it("reports connection error text from ext diagnostics", () => {
    const cfg = { source: "BalansunPeer", ext_peer_ip: "1.1.1.1" } as RouterConfig;
    const d = {
      source: "BalansunPeer",
      date_valid: false,
      ext: { last_error: "timeout", last_poll_ok: false },
    } as SourceDiagnostics;
    const conn = buildSourceSummaryRows(cfg, d, en).find(
      (r) => r.label === en.settings.sourceSummary.connection,
    );
    expect(conn?.status).toBe("err");
    expect(conn?.value).toContain("timeout");
  });

  it("shows effective meter when different from configured active source", () => {
    const cfg = { source: "JsyMk194" } as RouterConfig;
    const d = {
      source: "JsyMk194",
      diagnostics: {
        active_source: "JsyMk194",
        meter: "linky",
        last_poll_ok: false,
      },
      date_valid: false,
    } as SourceDiagnostics;
    const rows = buildSourceSummaryRows(cfg, d, en);
    expect(rows.some((r) => r.label === en.settings.sourceSummary.effectiveMeter)).toBe(true);
  });

  it("includes enphase user and serial rows", () => {
    const cfg = { source: "Enphase", ext_peer_ip: "192.168.1.80" } as RouterConfig;
    const d = {
      source: "Enphase",
      enphase: { user: "site", serial: "ABC" },
    } as SourceDiagnostics;
    const rows = buildSourceSummaryRows(cfg, d, en);
    expect(rows.some((r) => r.label === en.settings.sourceSummary.enphaseUser)).toBe(true);
    expect(rows.some((r) => r.label === en.settings.sourceSummary.enphaseSerial)).toBe(true);
  });

  it("includes jsy mk333 baud when set", () => {
    const cfg = { source: "JsyMk333", jsy_mk333_serial_baud: 57600 } as RouterConfig;
    const rows = buildSourceSummaryRows(cfg, null, en);
    expect(rows.some((r) => r.label === en.settings.sourceSummary.jsyMk333Baud)).toBe(true);
  });

  it("includes voltage and frequency for physical meter with measurements", () => {
    const cfg = { source: "Linky" } as RouterConfig;
    const d = {
      source: "Linky",
      voltage_house_v: 230.5,
      frequency_hz: 50,
    } as SourceDiagnostics;
    const rows = buildSourceSummaryRows(cfg, d, en);
    expect(rows.some((r) => r.label === en.settings.sourceSummary.voltage)).toBe(true);
    expect(rows.some((r) => r.label === en.settings.sourceSummary.frequency)).toBe(true);
  });

  it("lan source uses ext peer ip from diagnostics when config unset", () => {
    const cfg = { source: "ShellyEm", ext_peer_ip: "0.0.0.0" } as RouterConfig;
    const d = {
      source: "ShellyEm",
      ext: { ext_peer_ip: "192.168.1.55" },
    } as SourceDiagnostics;
    const ip = buildSourceSummaryRows(cfg, d, en).find(
      (r) => r.label === en.settings.sourceSummary.targetIp,
    );
    expect(ip?.value).toBe("192.168.1.55");
  });

  it("connection stale without error uses warn status", () => {
    const cfg = { source: "JsyMk194" } as RouterConfig;
    const d = {
      source: "JsyMk194",
      date_valid: false,
      diagnostics: { last_poll_ok: false },
    } as SourceDiagnostics;
    const conn = buildSourceSummaryRows(cfg, d, en).find(
      (r) => r.label === en.settings.sourceSummary.connection,
    );
    expect(conn?.status).toBe("warn");
    expect(conn?.value).toContain(en.settings.sourceSummary.statusStale);
  });

  it("ext protocol used row when diagnostics report protocol_used", () => {
    const cfg = { source: "BalansunPeer", ext_peer_ip: "1.2.3.4" } as RouterConfig;
    const d = {
      source: "BalansunPeer",
      diagnostics: { protocol_used: "json" },
    } as SourceDiagnostics;
    const rows = buildSourceSummaryRows(cfg, d, en);
    expect(rows.some((r) => r.label === en.settings.sourceSummary.protocolUsed)).toBe(true);
  });

  it("connection ok uses ext poll when diagnostics absent", () => {
    const cfg = { source: "BalansunPeer", ext_peer_ip: "1.1.1.1" } as RouterConfig;
    const d = {
      source: "BalansunPeer",
      date_valid: true,
      ext: { last_poll_ok: true, last_poll_ms_ago: 500 },
    } as SourceDiagnostics;
    const conn = buildSourceSummaryRows(cfg, d, en).find(
      (r) => r.label === en.settings.sourceSummary.connection,
    );
    expect(conn?.status).toBe("ok");
    expect(conn?.value).toContain("500");
  });

  it("includes source_data row when peer meter differs", () => {
    const cfg = { source: "BalansunPeer", ext_peer_ip: "1.2.3.4" } as RouterConfig;
    const d = {
      source: "BalansunPeer",
      diagnostics: { source_data: "JsyMk194" },
    } as SourceDiagnostics;
    const row = buildSourceSummaryRows(cfg, d, en).find(
      (r) => r.label === en.settings.sourceSummary.sourceData,
    );
    expect(row?.value).toContain("JsyMk194");
  });

  it("includes uxi calibration rows", () => {
    const cfg = { source: "Analog", calib_u: 1100, calib_i: 900 } as RouterConfig;
    const rows = buildSourceSummaryRows(cfg, null, en);
    expect(rows.some((r) => r.label === en.settings.sourceSummary.calibU)).toBe(true);
    expect(rows.some((r) => r.label === en.settings.sourceSummary.calibI)).toBe(true);
  });

  it("uses diagnostics mains frequency when frequency_hz missing", () => {
    const cfg = { source: "JsyMk194" } as RouterConfig;
    const d = {
      source: "JsyMk194",
      voltage_house_v: 230,
      diagnostics: { mains_frequency_hz: 49.9 },
    } as SourceDiagnostics;
    const rows = buildSourceSummaryRows(cfg, d, en);
    expect(rows.some((r) => r.label === en.settings.sourceSummary.frequency)).toBe(true);
  });

  it("includes linky tariff row when ltarf present", () => {
    const cfg = { source: "Linky" } as RouterConfig;
    const d = { source: "Linky", linky: { ltarf: "HPHC", cacsi_no_export: false } } as SourceDiagnostics;
    const rows = buildSourceSummaryRows(cfg, d, en);
    const tariff = rows.find((r) => r.label === en.settings.sourceSummary.linkyTariff);
    expect(tariff?.value).toBe("HPHC");
  });

  it("sourceFriendlyTitle uses notdef for blank wire id", () => {
    expect(sourceFriendlyTitle("", en)).toBe(en.settings.sourceSummary.titles.notdef);
  });

  it("configured row uses NotDef when no source on cfg or diagnostics", () => {
    const rows = buildSourceSummaryRows({} as RouterConfig, null, en);
    expect(rows[0]?.value).toContain("NotDef");
  });

  it("peer URL from ext diagnostics normalizes path without leading slash", () => {
    const cfg = { source: "BalansunPeer" } as RouterConfig;
    const d = {
      source: "BalansunPeer",
      ext: { ext_peer_ip: "10.0.0.2", ext_peer_port: 443, ext_peer_path: "meter" },
    } as SourceDiagnostics;
    const peer = buildSourceSummaryRows(cfg, d, en).find(
      (r) => r.label === en.settings.sourceSummary.peerUrl,
    );
    expect(peer?.value).toBe("10.0.0.2:443/meter");
  });

  it("connection error prefers diagnostics.last_error over ext", () => {
    const cfg = { source: "JsyMk194" } as RouterConfig;
    const d = {
      source: "JsyMk194",
      date_valid: false,
      diagnostics: { last_poll_ok: false, last_error: "meter fault" },
      ext: { last_error: "ignored" },
    } as SourceDiagnostics;
    const conn = buildSourceSummaryRows(cfg, d, en).find(
      (r) => r.label === en.settings.sourceSummary.connection,
    );
    expect(conn?.value).toContain("meter fault");
  });

  it("omits effective meter when meter matches active source", () => {
    const cfg = { source: "JsyMk194" } as RouterConfig;
    const d = {
      source: "JsyMk194",
      diagnostics: { active_source: "JsyMk194", meter: "JsyMk194" },
    } as SourceDiagnostics;
    const rows = buildSourceSummaryRows(cfg, d, en);
    expect(rows.some((r) => r.label === en.settings.sourceSummary.effectiveMeter)).toBe(false);
  });

  it("includes source_data from top-level diagnostics field", () => {
    const cfg = { source: "BalansunPeer", ext_peer_ip: "1.2.3.4" } as RouterConfig;
    const d = { source: "BalansunPeer", source_data: "Analog" } as SourceDiagnostics;
    const row = buildSourceSummaryRows(cfg, d, en).find(
      (r) => r.label === en.settings.sourceSummary.sourceData,
    );
    expect(row?.value).toContain("Analog");
  });

  it("uxi calibration defaults to 1000 when unset", () => {
    const cfg = { source: "Analog" } as RouterConfig;
    const rows = buildSourceSummaryRows(cfg, null, en);
    const calU = rows.find((r) => r.label === en.settings.sourceSummary.calibU);
    const calI = rows.find((r) => r.label === en.settings.sourceSummary.calibI);
    expect(calU?.value).toBe("1000");
    expect(calI?.value).toBe("1000");
  });

  it("formatWireLine uses NotDef for whitespace-only configured source", () => {
    const rows = buildSourceSummaryRows(
      { source: "BalansunPeer" } as RouterConfig,
      { source: "   " } as SourceDiagnostics,
      en,
    );
    expect(rows[0]?.value).toContain("NotDef");
  });

  it("ext peer URL from empty ext object returns dash", () => {
    const cfg = { source: "BalansunPeer" } as RouterConfig;
    const d = { source: "BalansunPeer", ext: {} } as SourceDiagnostics;
    const peer = buildSourceSummaryRows(cfg, d, en).find(
      (r) => r.label === en.settings.sourceSummary.peerUrl,
    );
    expect(peer?.value).toBe("—");
  });

  it("ext peer URL from ext block uses default port and slash path", () => {
    const cfg = { source: "BalansunPeer" } as RouterConfig;
    const d = {
      source: "BalansunPeer",
      ext: { ext_peer_ip: "10.0.0.8", ext_peer_path: "data" },
    } as SourceDiagnostics;
    const peer = buildSourceSummaryRows(cfg, d, en).find(
      (r) => r.label === en.settings.sourceSummary.peerUrl,
    );
    expect(peer?.value).toBe("10.0.0.8:80/data");
  });

  it("uses configured wire id when source is in registry", () => {
    const cfg = { source: "BalansunPeer", ext_peer_ip: "10.0.0.2" } as RouterConfig;
    const d = {
      source: "BalansunPeer",
      ext: { ext_peer_ip: "10.0.0.2", ext_peer_path: "/m" },
    } as SourceDiagnostics;
    const peer = buildSourceSummaryRows(cfg, d, en).find(
      (r) => r.label === en.settings.sourceSummary.peerUrl,
    );
    expect(peer?.value).toContain("10.0.0.2");
  });

  it("sourceKey falls back to cfg.source when device source empty", () => {
    const cfg = { source: "BalansunPeer", ext_peer_ip: "9.9.9.9" } as RouterConfig;
    const rows = buildSourceSummaryRows(cfg, {} as SourceDiagnostics, en);
    const peer = rows.find((r) => r.label === en.settings.sourceSummary.peerUrl);
    expect(peer?.value).toContain("9.9.9.9");
  });

  it("wire id falls back to cfg.source for ext rows", () => {
    const cfg = { source: "BalansunPeer", ext_peer_ip: "1.2.3.4" } as RouterConfig;
    const d = { source: "NotDef" } as SourceDiagnostics;
    const peer = buildSourceSummaryRows(cfg, d, en).find(
      (r) => r.label === en.settings.sourceSummary.peerUrl,
    );
    expect(peer?.value).toBe("1.2.3.4:80/");
  });

  it("shows effective meter when diagnostics meter differs from active source", () => {
    const cfg = { source: "BalansunPeer" } as RouterConfig;
    const d = {
      source: "BalansunPeer",
      diagnostics: { active_source: "BalansunPeer", meter: "JsyMk194" },
    } as SourceDiagnostics;
    const rows = buildSourceSummaryRows(cfg, d, en);
    expect(rows.some((r) => r.label === en.settings.sourceSummary.effectiveMeter)).toBe(true);
  });

  it("ext peer uses slash-prefixed path from ext block", () => {
    const cfg = { source: "BalansunPeer" } as RouterConfig;
    const d = {
      source: "BalansunPeer",
      ext: { ext_peer_ip: "10.0.0.3", ext_peer_path: "/ready" },
    } as SourceDiagnostics;
    const peer = buildSourceSummaryRows(cfg, d, en).find(
      (r) => r.label === en.settings.sourceSummary.peerUrl,
    );
    expect(peer?.value).toBe("10.0.0.3:80/ready");
  });

  it("ext peer defaults port 80 when ext_peer_port is zero", () => {
    const cfg = { source: "BalansunPeer" } as RouterConfig;
    const d = {
      source: "BalansunPeer",
      ext: { ext_peer_ip: "10.0.0.9", ext_peer_port: 0, ext_peer_path: "v1" },
    } as SourceDiagnostics;
    const peer = buildSourceSummaryRows(cfg, d, en).find(
      (r) => r.label === en.settings.sourceSummary.peerUrl,
    );
    expect(peer?.value).toBe("10.0.0.9:80/v1");
  });

  it("ext peer defaults path to slash when ext_peer_path missing", () => {
    const cfg = { source: "BalansunPeer" } as RouterConfig;
    const d = {
      source: "BalansunPeer",
      ext: { ext_peer_ip: "10.1.0.1" },
    } as SourceDiagnostics;
    const peer = buildSourceSummaryRows(cfg, d, en).find(
      (r) => r.label === en.settings.sourceSummary.peerUrl,
    );
    expect(peer?.value).toBe("10.1.0.1:80/");
  });

  it("ext peer keeps explicit non-zero port", () => {
    const cfg = { source: "BalansunPeer" } as RouterConfig;
    const d = {
      source: "BalansunPeer",
      ext: { ext_peer_ip: "10.0.0.9", ext_peer_port: 8080, ext_peer_path: "v1" },
    } as SourceDiagnostics;
    const peer = buildSourceSummaryRows(cfg, d, en).find(
      (r) => r.label === en.settings.sourceSummary.peerUrl,
    );
    expect(peer?.value).toBe("10.0.0.9:8080/v1");
  });

  it("ext peer URL uses config when diagnostics ext block missing", () => {
    const cfg = {
      source: "BalansunPeer",
      ext_peer_ip: "192.168.0.10",
      ext_peer_port: 80,
      ext_peer_path: "",
    } as RouterConfig;
    const d = { source: "BalansunPeer" } as SourceDiagnostics;
    const peer = buildSourceSummaryRows(cfg, d, en).find(
      (r) => r.label === en.settings.sourceSummary.peerUrl,
    );
    expect(peer?.value).toBe("192.168.0.10:80/");
  });
});
