const DEFAULT_APP_NAME = "Balansun";
const DEFAULT_SHORT_NAME = "Balansun";

export const PWA_MANIFEST_PATH = "/manifest.webmanifest";
export const PWA_ICON_192_PATH = "/pwa/icon-192.png";
export const PWA_ICON_512_PATH = "/pwa/icon-512.png";

export interface WebManifestShape {
  name: string;
  short_name: string;
  description: string;
  start_url: string;
  scope: string;
  id: string;
  display: string;
  display_override?: string[];
  background_color: string;
  theme_color: string;
  icons: Array<{
    src: string;
    sizes: string;
    type: string;
    purpose: string;
  }>;
}

function pwaColorsFromDocument(): { theme: string; background: string } {
  if (typeof document === "undefined") {
    return { theme: "#f5f6f9", background: "#f5f6f9" };
  }
  const root = document.documentElement;
  const theme =
    getComputedStyle(root).getPropertyValue("--pwa-theme-color").trim() || "#f5f6f9";
  const background =
    getComputedStyle(root).getPropertyValue("--pwa-background-color").trim() || theme;
  return { theme, background };
}

export function buildWebManifest(routerName?: string): WebManifestShape {
  const name = (routerName || "").trim() || DEFAULT_APP_NAME;
  const shortName =
    name.length <= 12 ? name : name.startsWith("Balansun") ? DEFAULT_SHORT_NAME : name.slice(0, 12);
  const { theme, background } = pwaColorsFromDocument();
  return {
    name,
    short_name: shortName,
    description: "Balansun — embedded web interface",
    start_url: "/",
    scope: "/",
    id: "/",
    display: "fullscreen",
    display_override: ["fullscreen", "standalone"],
    background_color: background,
    theme_color: theme,
    icons: [
      {
        src: PWA_ICON_192_PATH,
        sizes: "192x192",
        type: "image/png",
        purpose: "any",
      },
      {
        src: PWA_ICON_512_PATH,
        sizes: "512x512",
        type: "image/png",
        purpose: "maskable",
      },
    ],
  };
}

function upsertLink(rel: string, attrs: Record<string, string>): HTMLLinkElement {
  let link = document.querySelector<HTMLLinkElement>(`link[rel="${rel}"]`);
  if (!link) {
    link = document.createElement("link");
    link.rel = rel;
    document.head.appendChild(link);
  }
  for (const [k, v] of Object.entries(attrs)) {
    link.setAttribute(k, v);
  }
  return link;
}

function upsertMeta(name: string, content: string): void {
  let meta = document.querySelector<HTMLMetaElement>(`meta[name="${name}"]`);
  if (!meta) {
    meta = document.createElement("meta");
    meta.name = name;
    document.head.appendChild(meta);
  }
  meta.content = content;
}

/** Install or refresh manifest + Apple PWA head tags. */
export function installPwaManifest(routerName?: string): void {
  const manifest = buildWebManifest(routerName);
  upsertLink("manifest", { href: PWA_MANIFEST_PATH });
  upsertMeta("theme-color", manifest.theme_color);
  upsertMeta("apple-mobile-web-app-capable", "yes");
  upsertMeta("apple-mobile-web-app-status-bar-style", "black-translucent");
  upsertMeta("apple-mobile-web-app-title", manifest.short_name);
  upsertLink("apple-touch-icon", { href: PWA_ICON_192_PATH });
}

export function embeddedUiVersion(): string {
  const meta = document.querySelector<HTMLMetaElement>('meta[name="balansun-ui-version"]');
  const fromMeta = meta?.content?.trim();
  if (fromMeta && !fromMeta.includes("%VITE_")) return fromMeta;
  const fromEnv = import.meta.env.VITE_FIRMWARE_VERSION?.trim();
  if (fromEnv && fromEnv !== "dev") return fromEnv;
  return fromMeta || fromEnv || "dev";
}
