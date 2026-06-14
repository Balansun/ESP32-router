import { describe, expect, it } from "vitest";
import { previewBinding } from "../src/utils/pmqttPreview";

describe("pmqttPreview", () => {
  it("parses plain numeric payload", () => {
    const res = previewBinding(
      { metric: "house.signed_net_w", topic: "a", format: "plain", enabled: true },
      "-1500",
    );
    expect(res.ok).toBe(true);
    expect(res.value).toBe(-1500);
  });

  it("parses nested json path with $. prefix", () => {
    const res = previewBinding(
      {
        metric: "house.active_export_w",
        topic: "a",
        format: "json",
        path: "$.house.active_export_w",
        enabled: true,
      },
      '{"house":{"active_export_w":2100}}',
    );
    expect(res.ok).toBe(true);
    expect(res.value).toBe(2100);
    expect(res.displayValue).toBe("2,100 W");
  });

  it("formats energy binding with kWh decimals", () => {
    const res = previewBinding(
      {
        metric: "house.energy_day_import_wh",
        topic: "a",
        format: "json",
        path: "EnergieJour_M_Soutiree",
        enabled: true,
      },
      '{"EnergieJour_M_Soutiree":3011.7}',
    );
    expect(res.ok).toBe(true);
    expect(res.displayValue).toBe("3.01 kWh");
  });

  it("returns parse error for missing json field", () => {
    const res = previewBinding(
      {
        metric: "house.active_export_w",
        topic: "a",
        format: "json",
        path: "$.house.active_export_w",
        enabled: true,
      },
      '{"house":{"active_import_w":10}}',
    );
    expect(res.ok).toBe(false);
  });

  it("rejects invalid json payload", () => {
    const res = previewBinding(
      {
        metric: "house.signed_net_w",
        topic: "a",
        format: "json",
        path: "$.p",
        enabled: true,
      },
      "not-json",
    );
    expect(res.error).toBe("json_parse");
  });

  it("snapshot format accepts object at path", () => {
    const res = previewBinding(
      {
        metric: "house.signed_net_w",
        topic: "a",
        format: "snapshot",
        path: "$.house",
        enabled: true,
      },
      '{"house":{"Pw":-1}}',
    );
    expect(res.displayValue).toBe("snapshot_ok");
  });

  it("warn_sign for positive signed_net_w in json payload", () => {
    const res = previewBinding(
      {
        metric: "house.signed_net_w",
        topic: "a",
        format: "json",
        path: "$.p",
        enabled: true,
      },
      '{"p":500}',
    );
    expect(res.hint).toBe("warn_sign");
  });
});
