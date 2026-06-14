import { describe, expect, it } from "vitest";
import catalog from "../../public/firmware-catalog.json";
import {
  catalogEntryLabel,
  catalogFamilyLabel,
  entriesMatchFlashed,
  releaseFirmwareArtifactName,
  resolveFlashedCatalogEntry,
  type FirmwareCatalog,
} from "./productProfile";

const CATALOG = catalog as FirmwareCatalog;

describe("resolveFlashedCatalogEntry", () => {
  it("matches jsy_mk194_router", () => {
    const entry = resolveFlashedCatalogEntry(CATALOG, undefined, "jsy_mk194_router");
    expect(entry?.pio_env).toBe("jsy_mk194_router");
    expect(entry?.family).toBe("router");
  });

  it("matches full_router with meter_pack full to wroom32", () => {
    const entry = resolveFlashedCatalogEntry(
      CATALOG,
      { meter_pack: "full" },
      "full_router",
    );
    expect(entry?.pio_env).toBe("wroom32");
  });

  it("matches meter profile via pack declination", () => {
    const entry = resolveFlashedCatalogEntry(
      CATALOG,
      { meter_pack: "linky" },
      "linky_meter",
    );
    expect(entry?.pio_env).toBe("linky_meter");
    expect(entry?.family).toBe("meter");
  });

  it("matches router profile via pack when product_profile is generic router", () => {
    const entry = resolveFlashedCatalogEntry(
      CATALOG,
      { meter_pack: "jsy_mk194" },
      "jsy_mk194_router",
    );
    expect(entry?.pio_env).toBe("jsy_mk194_router");
  });

  it("returns undefined when profile is missing", () => {
    expect(resolveFlashedCatalogEntry(CATALOG, undefined, "")).toBeUndefined();
  });
});

describe("catalog labels and matching", () => {
  it("picks locale-specific labels", () => {
    const entry = CATALOG.entries[0]!;
    expect(catalogEntryLabel(entry, "fr")).toBe(entry.label_fr);
    expect(catalogEntryLabel(entry, "en")).toBe(entry.label_en);
    const family = CATALOG.families[0]!;
    expect(catalogFamilyLabel(family, "fr")).toBe(family.label_fr);
    expect(catalogFamilyLabel(family, "en")).toBe(family.label_en);
  });

  it("compares flashed and selected catalog rows by pio_env", () => {
    const a = CATALOG.entries[0]!;
    const b = CATALOG.entries[1]!;
    expect(entriesMatchFlashed(a, a)).toBe(true);
    expect(entriesMatchFlashed(a, b)).toBe(false);
    expect(entriesMatchFlashed(undefined, a)).toBe(false);
  });
});

describe("releaseFirmwareArtifactName", () => {
  it("formats release binary name", () => {
    expect(releaseFirmwareArtifactName("1.2.3", "linky_meter")).toBe(
      "balansun-1.2.3-linky_meter-firmware.bin",
    );
  });
});
