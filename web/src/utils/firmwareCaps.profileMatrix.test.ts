import { beforeEach, describe, expect, it } from "vitest";
import { deviceInfo } from "../state/store";
import { productModePioEnv } from "../state/productMode";
import {
  compiledMeterWires,
  hasCompiledMeter,
  hasMultiAction,
  hasSurplusRegulation,
  hasTriac,
  isSingleMeterProfile,
  meterPackId,
  showAdvancedMeterSourcesSection,
} from "./firmwareCaps";
import {
  mockDeviceFromRow,
  profileMatrixVariants,
} from "../../test/utils/profileMatrix";

describe("firmwareCaps profile matrix", () => {
  beforeEach(() => {
    deviceInfo.set(undefined);
    productModePioEnv.set(null);
  });

  it.each(profileMatrixVariants.map((row) => [row.product_profile, row] as const))(
    "%s caps and gates",
    (_profile, row) => {
      deviceInfo.set(mockDeviceFromRow(row));

      expect(hasSurplusRegulation()).toBe(row.caps.surplus_regulation);
      expect(hasTriac()).toBe(row.caps.triac);
      expect(hasMultiAction()).toBe(row.caps.multi_action);
      expect(isSingleMeterProfile()).toBe(row.ui.single_meter);
      expect(compiledMeterWires()).toEqual(row.meters);
      expect(meterPackId()).toBe(row.pack_id === "full" ? "full" : row.pack_id);

      for (const wire of row.meters) {
        expect(hasCompiledMeter(wire)).toBe(true);
      }
      const absentWire = ["Linky", "JsyMk194", "ShellyEm", "Pmqtt"].find(
        (w) => !row.meters.includes(w),
      );
      if (absentWire) {
        expect(hasCompiledMeter(absentWire)).toBe(false);
      }

      expect(showAdvancedMeterSourcesSection()).toBe(row.ui.show_advanced_meter_sources);
    },
  );
});
