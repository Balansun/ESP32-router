import type { FirmwareCapabilities } from "../api/types";
import { firmwareCapabilities, meterPackId, productProfileId } from "./firmwareCaps";

export type ProductFamilyId = "meter" | "router" | "full";

export interface FirmwareCatalogFamily {
  id: ProductFamilyId;
  label_fr: string;
  label_en: string;
}

export interface FirmwareCatalogEntry {
  family: ProductFamilyId;
  declination_id: string;
  label_fr: string;
  label_en: string;
  pio_env: string;
  product_profile: string;
}

export interface FirmwareCatalog {
  version: number;
  families: FirmwareCatalogFamily[];
  entries: FirmwareCatalogEntry[];
}

export function catalogEntryLabel(entry: FirmwareCatalogEntry, locale: "fr" | "en"): string {
  return locale === "fr" ? entry.label_fr : entry.label_en;
}

export function catalogFamilyLabel(
  family: FirmwareCatalogFamily,
  locale: "fr" | "en",
): string {
  return locale === "fr" ? family.label_fr : family.label_en;
}

/** Match flashed firmware to a catalog row (compile-time profile). */
export function resolveFlashedCatalogEntry(
  catalog: FirmwareCatalog,
  caps?: FirmwareCapabilities,
  productProfile?: string,
): FirmwareCatalogEntry | undefined {
  const profile = productProfile ?? productProfileId();
  const fw = caps ?? firmwareCapabilities();
  if (!profile) return undefined;

  if (profile === "full_router") {
    const pack = fw?.meter_pack ?? meterPackId();
    if (pack === "full" || !pack) {
      return catalog.entries.find((e) => e.pio_env === "wroom32");
    }
  }

  const direct = catalog.entries.find((e) => e.product_profile === profile);
  if (direct) return direct;

  const pack = fw?.meter_pack ?? meterPackId();
  if (pack && profile.endsWith("_router")) {
    return catalog.entries.find(
      (e) => e.family === "router" && e.declination_id === pack,
    );
  }
  if (pack && profile.endsWith("_meter")) {
    return catalog.entries.find(
      (e) => e.family === "meter" && e.declination_id === pack,
    );
  }
  return undefined;
}

export function catalogEntriesForFamily(
  catalog: FirmwareCatalog,
  familyId: ProductFamilyId,
): FirmwareCatalogEntry[] {
  return catalog.entries.filter((e) => e.family === familyId);
}

export function releaseFirmwareArtifactName(version: string, pioEnv: string): string {
  const safe = version.trim() || "unknown";
  return `balansun-${safe}-${pioEnv}-firmware.bin`;
}

export function entriesMatchFlashed(
  flashed: FirmwareCatalogEntry | undefined,
  selected: FirmwareCatalogEntry | undefined,
): boolean {
  if (!flashed || !selected) return false;
  return flashed.pio_env === selected.pio_env;
}
