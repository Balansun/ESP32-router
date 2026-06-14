/** Inlined for firmware: only the gzipped SPA is served (no /brand/* static files). */
import faviconSvg from "../../../assets/brand/balansun-favicon.svg?raw";
import logoDarkSvg from "../../../assets/brand/balansun-logo-dark.svg?raw";
import logoLightSvg from "../../../assets/brand/balansun-logo-light.svg?raw";
import type { ThemePref } from "../state/store";

function svgDataUrl(svg: string): string {
  return `data:image/svg+xml,${encodeURIComponent(svg)}`;
}

export type BrandTheme = "light" | "dark";

const faviconDataUrl = svgDataUrl(faviconSvg);

/** Effective light/dark for brand assets (ignores system/auto). */
export function brandThemeFromPref(pref: ThemePref): BrandTheme {
  if (pref === "dark") return "dark";
  if (pref === "light") return "light";
  if (typeof window !== "undefined" && window.matchMedia("(prefers-color-scheme: dark)").matches) {
    return "dark";
  }
  return "light";
}

export function brandFaviconDataUrl(_theme: BrandTheme): string {
  return faviconDataUrl;
}

export function brandLogoDataUrl(theme: BrandTheme): string {
  return svgDataUrl(theme === "dark" ? logoDarkSvg : logoLightSvg);
}

export function installBrandAssets(_theme: BrandTheme): void {
  let icon = document.querySelector<HTMLLinkElement>('link[rel="icon"]');
  if (!icon) {
    icon = document.createElement("link");
    icon.rel = "icon";
    icon.type = "image/svg+xml";
    document.head.appendChild(icon);
  }
  icon.href = faviconDataUrl;
}

export function applyBrandAssets(pref: ThemePref): BrandTheme {
  const theme = brandThemeFromPref(pref);
  installBrandAssets(theme);
  return theme;
}
