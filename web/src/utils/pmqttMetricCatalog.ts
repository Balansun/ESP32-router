import type { PmqttBinding } from "../api/types";

export type MetricTier = "required" | "recommended" | "optional";
export type PmqttScenarioId = "signed_power" | "house_snapshot" | "split_import_export" | "custom";

export interface MetricDef {
  id: string;
  label: string;
  unit: "W" | "VA" | "Wh" | "V" | "A" | "Hz" | "";
  tier: MetricTier;
  groups: string[];
}

export const REQUIRED_GROUP_SIGNED = "group_signed";
export const REQUIRED_GROUP_SPLIT = "group_split";
export const REQUIRED_GROUP_HOUSE_SNAPSHOT = "group_house_snapshot";

export const PMQTT_METRIC_CATALOG: MetricDef[] = [
  {
    id: "house.signed_net_w",
    label: "Puissance réseau signée",
    unit: "W",
    tier: "required",
    groups: [REQUIRED_GROUP_SIGNED],
  },
  { id: "house.pf", label: "Facteur de puissance", unit: "", tier: "recommended", groups: [] },
  {
    id: "house.snapshot",
    label: "Objet maison (snapshot)",
    unit: "",
    tier: "required",
    groups: [REQUIRED_GROUP_HOUSE_SNAPSHOT],
  },
  {
    id: "second.snapshot",
    label: "Objet second (snapshot)",
    unit: "",
    tier: "optional",
    groups: [],
  },
  {
    id: "house.active_import_w",
    label: "Soutirage maison",
    unit: "W",
    tier: "required",
    groups: [REQUIRED_GROUP_SPLIT],
  },
  {
    id: "house.active_export_w",
    label: "Injection maison",
    unit: "W",
    tier: "required",
    groups: [REQUIRED_GROUP_SPLIT],
  },
  { id: "house.apparent_import_va", label: "Soutirage apparent", unit: "VA", tier: "recommended", groups: [] },
  { id: "house.apparent_export_va", label: "Injection apparente", unit: "VA", tier: "recommended", groups: [] },
  { id: "house.energy_day_import_wh", label: "Energie jour import", unit: "Wh", tier: "optional", groups: [] },
  { id: "house.energy_day_export_wh", label: "Energie jour export", unit: "Wh", tier: "optional", groups: [] },
  { id: "house.energy_total_import_wh", label: "Energie totale import", unit: "Wh", tier: "optional", groups: [] },
  { id: "house.energy_total_export_wh", label: "Energie totale export", unit: "Wh", tier: "optional", groups: [] },
  { id: "second.active_import_w", label: "Second soutirage", unit: "W", tier: "optional", groups: [] },
  { id: "second.active_export_w", label: "Second injection", unit: "W", tier: "optional", groups: [] },
  { id: "second.apparent_import_va", label: "Second apparent import", unit: "VA", tier: "optional", groups: [] },
  { id: "second.apparent_export_va", label: "Second apparent export", unit: "VA", tier: "optional", groups: [] },
  { id: "second.energy_day_import_wh", label: "Second energie jour import", unit: "Wh", tier: "optional", groups: [] },
  { id: "second.energy_day_export_wh", label: "Second energie jour export", unit: "Wh", tier: "optional", groups: [] },
  { id: "second.energy_total_import_wh", label: "Second energie totale import", unit: "Wh", tier: "optional", groups: [] },
  { id: "second.energy_total_export_wh", label: "Second energie totale export", unit: "Wh", tier: "optional", groups: [] },
  { id: "raw_meter.voltage_house_v", label: "Tension maison", unit: "V", tier: "optional", groups: [] },
  { id: "raw_meter.current_house_a", label: "Courant maison", unit: "A", tier: "optional", groups: [] },
  { id: "raw_meter.pf_house", label: "PF maison", unit: "", tier: "optional", groups: [] },
  { id: "raw_meter.voltage_second_v", label: "Tension second", unit: "V", tier: "optional", groups: [] },
  { id: "raw_meter.current_second_a", label: "Courant second", unit: "A", tier: "optional", groups: [] },
  { id: "raw_meter.pf_second", label: "PF second", unit: "", tier: "optional", groups: [] },
  { id: "raw_meter.freq_hz", label: "Frequence reseau", unit: "Hz", tier: "optional", groups: [] },
  {
    id: "triac.open_percent",
    label: "Ouverture triac (%)",
    unit: "",
    tier: "optional",
    groups: [],
  },
];

export function metricDef(id: string): MetricDef | undefined {
  return PMQTT_METRIC_CATALOG.find((m) => m.id === id);
}

function activeBindings(bindings: PmqttBinding[]): PmqttBinding[] {
  return bindings.filter((b) => (b.enabled ?? true) && b.metric.trim() && b.topic.trim());
}

export function validateRequiredGroups(bindings: PmqttBinding[]): { ok: boolean; missing: string[] } {
  const active = activeBindings(bindings);
  const hasSigned = active.some((b) => b.metric === "house.signed_net_w");
  const hasSplit =
    active.some((b) => b.metric === "house.active_import_w") &&
    active.some((b) => b.metric === "house.active_export_w");
  const hasSnapshot = active.some((b) => b.metric === "house.snapshot");
  if (hasSigned || hasSplit || hasSnapshot) return { ok: true, missing: [] };
  return {
    ok: false,
    missing: [REQUIRED_GROUP_SIGNED, REQUIRED_GROUP_SPLIT, REQUIRED_GROUP_HOUSE_SNAPSHOT],
  };
}
