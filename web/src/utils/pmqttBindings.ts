import type { PmqttBinding, RouterConfig } from "../api/types";

/** Active bindings saved for source Pmqtt (EEPROM / GET config). */
export function pmqttActiveBindingCount(
  bindings: PmqttBinding[] | undefined,
): number {
  if (!Array.isArray(bindings)) return 0;
  return bindings.filter(
    (b) =>
      b &&
      b.enabled !== false &&
      String(b.metric || "").trim().length > 0 &&
      String(b.topic || "").trim().length > 0,
  ).length;
}

/** True when Pmqtt is selected but no bindings are persisted — MQTT JSON will not be mapped. */
export function pmqttBindingsMissing(
  config: Pick<RouterConfig, "source" | "pmqtt_bindings">,
): boolean {
  return config.source === "Pmqtt" && pmqttActiveBindingCount(config.pmqtt_bindings) === 0;
}

/**
 * Constrained-tier GET omits `pmqtt_bindings` while `pmqtt_bindings_on_device` is true.
 * Do not PATCH an empty array — that wipes EEPROM bindings and breaks later saves.
 */
export function omitPmqttBindingsFromConfigPatch(
  config: Pick<
    RouterConfig,
    "source" | "pmqtt_bindings" | "pmqtt_bindings_on_device"
  >,
  bindings: PmqttBinding[] | undefined,
): boolean {
  if (config.source !== "Pmqtt") return true;
  if (config.pmqtt_bindings_on_device && pmqttActiveBindingCount(bindings) === 0) {
    return true;
  }
  return false;
}
