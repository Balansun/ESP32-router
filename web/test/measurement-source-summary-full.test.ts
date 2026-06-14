import { describe, expect, it } from "vitest";
import { en } from "../src/i18n/locales/en";
import type { RouterConfig, SourceDiagnostics } from "../src/api/types";
import { buildSourceSummaryRows, sourceFriendlyTitle } from "../src/routes/settings/measurementSourceSummary";

describe("buildSourceSummaryRows full", () => {
  const T = en;

  it("shows effective meter when different from configured", () => {
    const rows = buildSourceSummaryRows(
      { source: "BalansunPeer" } as RouterConfig,
      {
        source: "BalansunPeer",
        diagnostics: { active_source: "BalansunPeer", meter: "JsyMk194" },
      } as SourceDiagnostics,
      T,
    );
    expect(rows.some((r) => r.label === T.settings.sourceSummary.effectiveMeter)).toBe(true);
  });

  it("connection error and stale branches", () => {
    const errRows = buildSourceSummaryRows(
      { source: "BalansunPeer" } as RouterConfig,
      {
        source: "BalansunPeer",
        diagnostics: { last_poll_ok: false, last_error: "timeout" },
      } as SourceDiagnostics,
      T,
    );
    expect(errRows.find((r) => r.label === T.settings.sourceSummary.connection)?.status).toBe("err");
    const staleRows = buildSourceSummaryRows(
      { source: "BalansunPeer" } as RouterConfig,
      { source: "BalansunPeer", diagnostics: { last_poll_ok: false } } as SourceDiagnostics,
      T,
    );
    expect(staleRows.find((r) => r.label === T.settings.sourceSummary.connection)?.status).toBe("warn");
    const extErr = buildSourceSummaryRows(
      { source: "BalansunPeer" } as RouterConfig,
      {
        source: "BalansunPeer",
        ext: { last_poll_ok: false, last_error: "peer down" },
      } as SourceDiagnostics,
      T,
    );
    expect(extErr.find((r) => r.label === T.settings.sourceSummary.connection)?.status).toBe("err");
  });

  it("ext peer from diagnostics ext block and protocol used", () => {
    const rows = buildSourceSummaryRows(
      { source: "BalansunPeer", ext_peer_ip: "0.0.0.0" } as RouterConfig,
      {
        source: "BalansunPeer",
        ext: {
          ext_peer_ip: "10.0.0.2",
          ext_peer_port: 8080,
          ext_peer_path: "measurements",
          protocol_used: "json",
        },
        diagnostics: { source_data: "JsyMk194", protocol_used: "json" },
      } as SourceDiagnostics,
      T,
    );
    expect(rows.some((r) => r.value.includes("10.0.0.2"))).toBe(true);
    expect(rows.some((r) => r.label === T.settings.sourceSummary.protocolUsed)).toBe(true);
    expect(rows.some((r) => r.label === T.settings.sourceSummary.sourceData)).toBe(true);
  });

  it("lan source target IP from config", () => {
    const rows = buildSourceSummaryRows(
      { source: "Enphase", ext_peer_ip: "192.168.1.5" } as RouterConfig,
      null,
      T,
    );
    expect(rows.some((r) => r.label === T.settings.sourceSummary.targetIp)).toBe(true);
  });

  it("enphase user and serial", () => {
    const rows = buildSourceSummaryRows(
      { source: "Enphase" } as RouterConfig,
      { source: "Enphase", enphase: { user: "u", serial: "s" } } as SourceDiagnostics,
      T,
    );
    expect(rows.some((r) => r.value === "u")).toBe(true);
  });

  it("jsy mk333 baud and analog calibration", () => {
    const uxi = buildSourceSummaryRows(
      { source: "Analog", calib_u: 1100, calib_i: 1200 } as RouterConfig,
      null,
      T,
    );
    expect(uxi.some((r) => r.label === T.settings.sourceSummary.calibU)).toBe(true);
    const x3 = buildSourceSummaryRows(
      { source: "JsyMk333", jsy_mk333_serial_baud: 115200 } as RouterConfig,
      null,
      T,
    );
    expect(x3.some((r) => r.value === "115200")).toBe(true);
  });

  it("physical meter voltage and frequency", () => {
    const rows = buildSourceSummaryRows(
      { source: "JsyMk194" } as RouterConfig,
      {
        source: "JsyMk194",
        voltage_house_v: 230.5,
        frequency_hz: 50.01,
      } as SourceDiagnostics,
      T,
    );
    expect(rows.some((r) => r.value.includes("230.5"))).toBe(true);
    expect(rows.some((r) => r.value.includes("50.01"))).toBe(true);
  });

  it("sourceFriendlyTitle covers wire variants", () => {
    expect(sourceFriendlyTitle("Analog", T)).toBe(T.settings.sourceSummary.titles.analog);
    expect(sourceFriendlyTitle("linky", T)).toBe(T.settings.sourceSummary.titles.linky);
    expect(sourceFriendlyTitle("enphase", T)).toBe(T.settings.sourceSummary.titles.enphase);
    expect(sourceFriendlyTitle("smartg", T)).toBe(T.settings.sourceSummary.titles.smartg);
    expect(sourceFriendlyTitle("shelly-em", T)).toBe(T.settings.sourceSummary.titles.shellyem);
    expect(sourceFriendlyTitle("BalansunPeer", T)).toBe(T.settings.sourceSummary.titles.balansunPeer);
    expect(sourceFriendlyTitle("JsyMk333", T)).toBe(T.settings.sourceSummary.titles.jsyMk333);
    expect(sourceFriendlyTitle("shellypro", T)).toBe(T.settings.sourceSummary.titles.shellypro);
    expect(sourceFriendlyTitle("homew", T)).toBe(T.settings.sourceSummary.titles.homew);
    expect(sourceFriendlyTitle("pmqtt", T)).toBe(T.settings.sourceSummary.titles.pmqtt);
    expect(sourceFriendlyTitle("notdef", T)).toBe(T.settings.sourceSummary.titles.notdef);
    expect(sourceFriendlyTitle("", T)).toBe(T.settings.sourceSummary.titles.notdef);
    expect(sourceFriendlyTitle("CustomWire", T)).toBe("CustomWire");
  });
});
