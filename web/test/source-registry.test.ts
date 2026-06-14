import { describe, expect, it } from "vitest";
import {
  SOURCE_REGISTRY,
  filterSourceRegistry,
  pinoutSectionUrl,
  registryEntry,
  resolveSourceWireId,
  spaSourceIdsFromFirmwareMeters,
  type SourceWireId,
} from "../src/routes/settings/sourceRegistry";

describe("source registry", () => {
  it("lists 11 configured sources", () => {
    expect(SOURCE_REGISTRY).toHaveLength(11);
    expect(registryEntry("Linky")?.pinoutAnchor).toBe("source_linky");
    expect(registryEntry("BalansunPeer")?.fields).toContain("ext_peer_path");
  });

  it("every entry has id, pinout anchor, and fields array", () => {
    for (const e of SOURCE_REGISTRY) {
      expect(e.id).toBeTruthy();
      expect(e.pinoutAnchor).toBeTruthy();
      expect(Array.isArray(e.fields)).toBe(true);
      expect(registryEntry(e.id)?.id).toBe(e.id);
    }
  });

  it("pinoutSectionUrl points at hardware pinout anchor", () => {
    const url = pinoutSectionUrl("source_linky");
    expect(url).toBe(
      "https://balansun.clouded.fr/en/hardware-pinout/#source_linky",
    );
  });

  it("filters registry by firmware meters wire ids", () => {
    const jsyOnly = filterSourceRegistry(["JsyMk194", "NotDef"]);
    expect(jsyOnly.map((e) => e.id)).toEqual(["JsyMk194"]);
    const extPeer = filterSourceRegistry(["BalansunPeer"]);
    expect(extPeer.map((e) => e.id)).toEqual(["BalansunPeer"]);
    expect(filterSourceRegistry(undefined)).toHaveLength(11);
    expect(filterSourceRegistry([])).toHaveLength(11);
  });

  it("maps canonical firmware meters to registry ids", () => {
    const ids = spaSourceIdsFromFirmwareMeters(["JsyMk194", "BalansunPeer", "Linky"]);
    expect(ids).toEqual(new Set<SourceWireId>(["JsyMk194", "BalansunPeer", "Linky"]));
  });

  it("rejects legacy wire labels", () => {
    expect(resolveSourceWireId("UxIx2")).toBeUndefined();
    expect(resolveSourceWireId("Ext")).toBeUndefined();
  });

  it("declares meter GPIO requirements on wired sources", () => {
    expect(registryEntry("JsyMk194")?.meterGpio).toEqual(["uart"]);
    expect(registryEntry("JsyMk194")?.uartPresenceId).toBe("jsy_mk194");
    expect(registryEntry("Analog")?.meterGpio).toEqual(["analog"]);
    expect(registryEntry("Linky")?.meterGpio).toEqual(["uart"]);
    expect(registryEntry("Enphase")?.meterGpio).toBeUndefined();
  });

  it("maps expected fields per source", () => {
    const expected: Partial<Record<SourceWireId, string[]>> = {
      Analog: ["calib_u", "calib_i"],
      Pmqtt: ["pmqtt_topic", "pmqtt_schema"],
      Enphase: ["enphase_user", "enphase_password", "enphase_serial"],
      ShellyPro: ["ext_peer_ip", "meter_channel"],
      JsyMk333: ["jsy_mk333_serial_baud"],
    };
    for (const [id, fields] of Object.entries(expected)) {
      expect(registryEntry(id as SourceWireId)?.fields).toEqual(fields);
    }
  });
});
