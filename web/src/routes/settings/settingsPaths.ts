export const SETTINGS_SECTIONS = [
  "product",
  "general",
  "metering",
  "network",
  "hardware",
  "led",
  "advanced",
] as const;

export type SettingsSection = (typeof SETTINGS_SECTIONS)[number];

export const SETTINGS_LAYOUT_ID = "settings";

export function isSettingsPath(path: string): boolean {
  const p = path.replace(/\/+$/, "") || "/";
  return p === "/settings" || p.startsWith("/settings/");
}

export function settingsPath(section: SettingsSection): string {
  return `/settings/${section}`;
}

export function parseSettingsSection(path: string): SettingsSection {
  const p = path.replace(/\/+$/, "") || "/";
  if (p === "/settings") return "general";
  const prefix = "/settings/";
  if (!p.startsWith(prefix)) return "general";
  const segment = p.slice(prefix.length).split("/")[0] ?? "";
  if ((SETTINGS_SECTIONS as readonly string[]).includes(segment)) {
    return segment as SettingsSection;
  }
  return "general";
}

/** Redirect bare /settings, unknown sections, or non-canonical paths to the section URL. */
export function normalizeSettingsRedirect(path: string): string | null {
  const p = path.replace(/\/+$/, "") || "/";
  if (!isSettingsPath(p)) return null;
  const section = parseSettingsSection(p);
  const canonical = settingsPath(section);
  return p === canonical ? null : canonical;
}
