import { getStrings } from "../i18n";
import { parseSettingsSection } from "../routes/settings/settingsPaths";

const ROUTE_TITLE_KEYS: Record<string, keyof ReturnType<typeof getStrings>["nav"] | "login"> = {
  "/": "home",
  "/history": "history",
  "/actions": "actions",
  "/settings": "settings",
  "/api": "api",
  "/backup": "backup",
  "/diag": "diag",
  "/firmware": "firmware",
  "/wifi": "wifi",
  "/wifi/station": "wifi",
  "/login": "login",
};

export function applyDocumentTitle(path: string): void {
  const T = getStrings();
  const p = path.replace(/\/+$/, "") || "/";
  if (p.startsWith("/settings")) {
    const section = parseSettingsSection(p);
    const sectionLabel = T.settings.tabs[section];
    document.title = `${sectionLabel} · ${T.settings.title} · ${T.appName}`;
    return;
  }
  const key = ROUTE_TITLE_KEYS[p];
  if (key === "login") {
    document.title = `${T.httpAuth.pageTitle} · ${T.appName}`;
    return;
  }
  const section = key ? T.nav[key] : "";
  document.title = section ? `${section} · ${T.appName}` : T.appName;
}
