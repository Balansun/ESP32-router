import { pinoutSectionUrl as pinoutDocUrl } from "../../fieldHelp/docUrl";

/** Wire IDs from firmware `kRegistry` (balansun_source.cpp), excluding NotDef as a target. */
export type SourceWireId =
  | "JsyMk194"
  | "JsyMk333"
  | "Analog"
  | "Linky"
  | "Enphase"
  | "ShellyEm"
  | "ShellyPro"
  | "SmartG"
  | "HomeW"
  | "Pmqtt"
  | "BalansunPeer";

/** GPIO groups a meter driver needs when it is the active `config.source`. */
export type MeterGpioGroup = "uart" | "analog";

export interface SourceRegistryEntry {
  id: SourceWireId;
  /** /en/hardware-pinout/ §17 subsection anchor. */
  pinoutAnchor: string;
  /** Pins customization: UART and/or ADC when this source is selected. */
  meterGpio?: MeterGpioGroup[];
  /** `GET /health` hardware item id for UART row status (JSY meters). */
  uartPresenceId?: string;
  fields: (
    | "ext_peer_ip"
    | "ext_peer_port"
    | "ext_peer_path"
    | "ext_protocol"
    | "enphase_user"
    | "enphase_password"
    | "enphase_serial"
    | "meter_channel"
    | "pmqtt_topic"
    | "pmqtt_schema"
    | "jsy_mk333_serial_baud"
    | "calib_u"
    | "calib_i"
  )[];
}

const LAN_HTTP = "17-sources-de-mesure--schémas-électrique-et-électronique";

export const SOURCE_REGISTRY: SourceRegistryEntry[] = [
  {
    id: "JsyMk194",
    pinoutAnchor: "source_jsy_mk194",
    meterGpio: ["uart"],
    uartPresenceId: "jsy_mk194",
    fields: [],
  },
  {
    id: "JsyMk333",
    pinoutAnchor: "source_jsy_mk333",
    meterGpio: ["uart"],
    uartPresenceId: "jsy_mk194",
    fields: ["jsy_mk333_serial_baud"],
  },
  {
    id: "Analog",
    pinoutAnchor: "source_analog",
    meterGpio: ["analog"],
    fields: ["calib_u", "calib_i"],
  },
  {
    id: "Linky",
    pinoutAnchor: "source_linky",
    meterGpio: ["uart"],
    fields: [],
  },
  { id: "Enphase", pinoutAnchor: "source_enphase", fields: ["enphase_user", "enphase_password", "enphase_serial"] },
  { id: "ShellyEm", pinoutAnchor: "source_shellyem", fields: ["ext_peer_ip"] },
  {
    id: "ShellyPro",
    pinoutAnchor: LAN_HTTP,
    fields: ["ext_peer_ip", "meter_channel"],
  },
  { id: "SmartG", pinoutAnchor: "source_smartg", fields: ["ext_peer_ip"] },
  { id: "HomeW", pinoutAnchor: LAN_HTTP, fields: ["ext_peer_ip"] },
  { id: "Pmqtt", pinoutAnchor: LAN_HTTP, fields: ["pmqtt_topic", "pmqtt_schema"] },
  {
    id: "BalansunPeer",
    pinoutAnchor: "source_ext",
    fields: ["ext_peer_ip", "ext_peer_port", "ext_peer_path"],
  },
];

function normalizeSourceKey(raw: string): string {
  return String(raw || "")
    .trim()
    .toLowerCase()
    .replace(/\s+/g, "")
    .replace(/-/g, "");
}

/** Map `config.source` / firmware wire → registry `SourceWireId` (canonical only). */
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

/** Resolve firmware `meters[]` to SPA source ids; `null` = no filter (show all). */
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

/** Filter wizard sources by `capabilities.firmware_capabilities.meters`. */
export function filterSourceRegistry(
  meters?: string[] | null,
): SourceRegistryEntry[] {
  const allowed = spaSourceIdsFromFirmwareMeters(meters);
  if (!allowed) return SOURCE_REGISTRY;
  return SOURCE_REGISTRY.filter((e) => allowed.has(e.id));
}
