import type { DeviceInfo } from "../api/types";
import { deviceInfo } from "./store";
import { syncProductModeFromDevice } from "./productMode";

/** Merge GET /device fields into the shared store (incl. firmware_capabilities). */
export function hydrateDeviceInfoFromApi(d: DeviceInfo): void {
  deviceInfo.set({
    router_name: d.router_name,
    firmware_version: d.firmware_version,
    probe_house_name: d.probe_house_name,
    probe_second_name: d.probe_second_name,
    temperature_label: d.temperature_label,
    capabilities: d.capabilities,
  });
  syncProductModeFromDevice();
}
