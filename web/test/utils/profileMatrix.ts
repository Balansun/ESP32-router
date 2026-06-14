export type { ProfileMatrixVariant } from "../../src/utils/profileMatrix";
export {
  profileMatrixVariants,
  profileVariantByPioEnv,
  profileVariantByProductProfile,
} from "../../src/utils/profileMatrix";

import type { DeviceInfo } from "../../src/api/types";
import type { ProfileMatrixVariant } from "../../src/utils/profileMatrix";

export function mockDeviceFromRow(row: ProfileMatrixVariant): DeviceInfo {
  return {
    router_name: "Balansun",
    firmware_version: "0.1.0",
    probe_house_name: "House",
    probe_second_name: "Second",
    temperature_label: "temperature_c",
    capabilities: {
      product_profile: row.product_profile,
      firmware_capabilities: {
        surplus_regulation: row.caps.surplus_regulation,
        triac: row.caps.triac,
        multi_action: row.caps.multi_action,
        meter_pack: row.pack_id === "full" ? "full" : row.pack_id,
        meters: row.meters,
        triac_dimming: row.caps.triac,
      },
    },
  };
}
