/** Factory MQTT device name before per-board device_uid default. */
export const FACTORY_MQTT_DEVICE_NAME = "balansun";

/** Effective MQTT topic device segment (defaults to device_uid). */
export function effectiveMqttDeviceName(stored: string, deviceUid: string): string {
  const trimmed = (stored ?? "").trim();
  if (trimmed.length > 0 && trimmed !== FACTORY_MQTT_DEVICE_NAME) {
    return trimmed;
  }
  const uid = String(deviceUid).trim();
  if (uid.length === 0) return FACTORY_MQTT_DEVICE_NAME;
  return uid;
}
