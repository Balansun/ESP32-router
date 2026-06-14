import { pinoutSectionUrl as pinoutDocUrl } from "../../fieldHelp/docUrl";
import {
  GENERATED_SOURCE_REGISTRY,
  type ConfigField,
  type MeterDriverBaseKind,
  type MeterSourceDefinition,
  type SourceWireId,
} from "./sourceRegistry.generated";

export type { ConfigField, MeterDriverBaseKind, MeterSourceDefinition, SourceWireId };

/** GPIO groups a meter driver needs when it is the active `config.source`. */
export type MeterGpioGroup = "uart" | "analog";

export interface SourceRegistryEntry {
  id: SourceWireId;
  pinoutAnchor: string;
  meterGpio?: MeterGpioGroup[];
  uartPresenceId?: string;
  baseKind: MeterDriverBaseKind;
  connectionKind: MeterSourceDefinition["connectionKind"];
  fields: ConfigField[];
}

export const SOURCE_REGISTRY: SourceRegistryEntry[] = GENERATED_SOURCE_REGISTRY.map((e) => ({
  id: e.id,
  pinoutAnchor: e.pinoutAnchor,
  meterGpio: e.meterGpio,
  uartPresenceId: e.uartPresenceId,
  baseKind: e.baseKind,
  connectionKind: e.connectionKind,
  fields: e.fields,
}));

function normalizeSourceKey(raw: string): string {
  return String(raw || "")
    .trim()
    .toLowerCase()
    .replace(/\s+/g, "")
    .replace(/-/g, "");
}

export function resolveSourceWireId(source: string): SourceWireId | undefined {
  const key = normalizeSourceKey(source);
  if (!key) return undefined;
  for (const entry of SOURCE_REGISTRY) {
    if (normalizeSourceKey(entry.id) === key) return entry.id;
  }
  return undefined;
}

export function pinoutSectionUrl(anchor: string): string {
  return pinoutDocUrl(anchor);
}

export function registryEntry(id: SourceWireId): SourceRegistryEntry | undefined {
  return SOURCE_REGISTRY.find((e) => e.id === id);
}

export function spaSourceIdsFromFirmwareMeters(
  meters: string[] | undefined | null,
): Set<SourceWireId> | null {
  if (!meters?.length) return null;
  const ids = new Set<SourceWireId>();
  for (const wire of meters) {
    const id = resolveSourceWireId(wire);
    if (id) ids.add(id);
  }
  return ids.size ? ids : null;
}

export function filterSourceRegistry(
  meters?: string[] | null,
): SourceRegistryEntry[] {
  const allowed = spaSourceIdsFromFirmwareMeters(meters);
  if (!allowed) return SOURCE_REGISTRY;
  return SOURCE_REGISTRY.filter((e) => allowed.has(e.id));
}

/** True when the source uses LAN HTTP polling (generated metadata). */
export function isLanHttpSource(id: SourceWireId): boolean {
  const e = registryEntry(id);
  return e?.baseKind === "lanHttpJson" || e?.baseKind === "lanHttpCustom" || e?.connectionKind === "https";
}
