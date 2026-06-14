import { describe, expect, it } from "vitest";
import { en } from "../src/i18n/locales/en";
import type { HardwarePresenceItem } from "../src/api/types";
import {
  alertVariantForState,
  hardwareDetailText,
  hardwareMetaLine,
  isRecheckable,
  partitionHardwareItems,
  pinoutAnchor,
} from "../src/routes/diag/hardwarePresenceUi";

describe("hardwarePresenceUi", () => {
  it("maps states to alert variants", () => {
    expect(alertVariantForState("missing")).toBe("danger");
    expect(alertVariantForState("paused")).toBe("warn");
    expect(alertVariantForState("ok")).toBe("success");
    expect(alertVariantForState("not_applicable")).toBe("muted");
  });

  it("partitions items into warnings, ok, and muted", () => {
    const items: HardwarePresenceItem[] = [
      { id: "zero_cross", role: "zero_cross", state: "missing" },
      { id: "triac_dim", role: "triac_dim", state: "ok" },
      { id: "jsy_mk194", role: "jsy_mk194", state: "not_applicable" },
    ];
    const p = partitionHardwareItems(items);
    expect(p.warnings.map((i) => i.id)).toEqual(["zero_cross"]);
    expect(p.ok.map((i) => i.id)).toEqual(["triac_dim"]);
    expect(p.muted.map((i) => i.id)).toEqual(["jsy_mk194"]);
  });

  it("localizes hardware detail text", () => {
    const item: HardwarePresenceItem = {
      id: "zero_cross",
      role: "zero_cross",
      state: "missing",
      gpio: 23,
      detail: "English fallback",
    };
    expect(hardwareDetailText(item, en)).toContain("No zero-cross");
    expect(hardwareMetaLine(item, en)).toContain("GPIO 23");
  });

  it("exposes pinout anchors for known ids", () => {
    expect(pinoutAnchor("zero_cross")).toContain("triac");
    expect(pinoutAnchor("jsy_mk194")).toBe("source-doc-uxix2");
    expect(pinoutAnchor("unknown")).toBeNull();
  });

  it("marks not_applicable as non-recheckable", () => {
    expect(isRecheckable({ id: "jsy_mk194", role: "jsy", state: "missing" })).toBe(true);
    expect(isRecheckable({ id: "jsy_mk194", role: "jsy", state: "not_applicable" })).toBe(false);
  });
});
