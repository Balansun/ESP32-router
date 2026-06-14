import { localePref } from "../state/store";
import type { FieldHelpTable } from "../i18n/locales/fieldHelp.en";

export type FieldHelpScope = keyof FieldHelpTable;

/**
 * Public documentation site origin (no trailing slash).
 * Set at SPA build time: VITE_DOCS_SITE_ORIGIN=https://balansun.clouded.fr
 */
export const DOCS_SITE_ORIGIN = (import.meta.env.VITE_DOCS_SITE_ORIGIN ?? "").replace(/\/$/, "");

/** @deprecated Use DOCS_SITE_ORIGIN */
export const FIELD_HELP_WEBSITE_BASE = DOCS_SITE_ORIGIN;

/** Absolute or root-relative docs URL. */
export function docsPageUrl(path: string): string {
  const p = path.startsWith("/") ? path : `/${path}`;
  return DOCS_SITE_ORIGIN ? `${DOCS_SITE_ORIGIN}${p}` : p;
}

export function docsLangHome(lang: "en" | "fr"): string {
  return docsPageUrl(`/${lang}/`);
}

const SCOPE_SLUG: Record<FieldHelpScope, string> = {
  settings: "settings",
  actions: "actions",
  firmware: "firmware",
  wifi: "wifi",
  sourceWizard: "source-wizard",
  httpAuth: "http-auth",
  api: "api",
  install: "install",
  backup: "backup",
};

/** Website URL for a field-help section (`/{lang}/field-help/{scope}/#{key}`). */
export function fieldHelpDocUrl(scope: FieldHelpScope, key: string, lang?: "en" | "fr"): string {
  const locale = lang ?? localePref.get();
  const docLang = locale === "fr" ? "fr" : "en";
  const slug = SCOPE_SLUG[scope];
  return docsPageUrl(`/${docLang}/field-help/${slug}/#${key}`);
}

export function pinoutSectionUrl(anchor: string, lang: "en" | "fr" = "en"): string {
  return docsPageUrl(`/${lang}/hardware-pinout/#${anchor}`);
}
