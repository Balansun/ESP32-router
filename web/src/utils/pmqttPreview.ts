import type { PmqttBinding } from "../api/types";
import { fmt2, fmtAmps, fmtEnergyWh, fmtPowerW, fmtVolts } from "./format";
import { getByJsonPath, normalizeJsonPath } from "./jsonPathIndex";
import { metricDef } from "./pmqttMetricCatalog";

export interface PmqttPreviewResult {
  ok: boolean;
  displayValue?: string;
  value?: number;
  error?: string;
  hint?: "warn_sign" | "warn_unit";
}

function formatMetricDisplay(metric: string, v: number): string {
  const def = metricDef(metric);
  const unit = def?.unit ?? "";
  if (unit === "Wh") {
    const e = fmtEnergyWh(v);
    return `${e.value} ${e.unit}`;
  }
  if (unit === "W" || unit === "VA") {
    return unit ? `${fmtPowerW(v)} ${unit}` : fmtPowerW(v);
  }
  if (unit === "V") return `${fmtVolts(v)} V`;
  if (unit === "A") return `${fmtAmps(v)} A`;
  if (unit === "Hz") return `${fmt2(v)} Hz`;
  if (metric.includes("pf")) return fmt2(v);
  return fmt2(v);
}

function toNumeric(v: unknown): number | null {
  if (typeof v === "number" && Number.isFinite(v)) return v;
  if (typeof v === "string") {
    const n = Number(v);
    if (Number.isFinite(n)) return n;
  }
  return null;
}

export function previewBinding(binding: PmqttBinding, payload: string): PmqttPreviewResult {
  const format = binding.format ?? "json";
  if (format === "plain") {
    const n = Number(payload.trim());
    if (!Number.isFinite(n)) return { ok: false, error: "plain_not_numeric" };
    const displayValue = formatMetricDisplay(binding.metric, n);
    return {
      ok: true,
      value: n,
      displayValue,
      hint: Math.abs(n) > 2_000_000 ? "warn_unit" : undefined,
    };
  }
  let parsed: unknown;
  try {
    parsed = JSON.parse(payload);
  } catch {
    return { ok: false, error: "json_parse" };
  }
  if (format === "snapshot") {
    const root = binding.path ? getByJsonPath(parsed, binding.path) : parsed;
    if (!root || typeof root !== "object") return { ok: false, error: "snapshot_path_not_object" };
    return { ok: true, displayValue: "snapshot_ok" };
  }
  const path = normalizeJsonPath(binding.path ?? "");
  const valueNode = getByJsonPath(parsed, path);
  const value = toNumeric(valueNode);
  if (value === null) return { ok: false, error: "json_value_not_numeric" };
  const hint: PmqttPreviewResult["hint"] =
    binding.metric === "house.signed_net_w" && value > 0
      ? "warn_sign"
      : Math.abs(value) > 2_000_000
        ? "warn_unit"
        : undefined;
  return { ok: true, value, displayValue: formatMetricDisplay(binding.metric, value), hint };
}
