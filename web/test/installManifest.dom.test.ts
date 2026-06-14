import { describe, expect, it } from "vitest";
import {
  buildWebManifest,
  installPwaManifest,
  PWA_ICON_192_PATH,
  PWA_ICON_512_PATH,
  PWA_MANIFEST_PATH,
} from "../src/pwa/installManifest";

describe("installManifest", () => {
  it("buildWebManifest includes URL icons and fullscreen display", () => {
    const manifest = buildWebManifest();
    expect(manifest.name).toBe("Balansun");
    expect(manifest.display).toBe("fullscreen");
    expect(manifest.display_override).toEqual(["fullscreen", "standalone"]);
    expect(manifest.start_url).toBe("/");
    expect(manifest.scope).toBe("/");
    expect(manifest.id).toBe("/");
    expect(manifest.icons).toHaveLength(2);
    expect(manifest.icons[0]?.src).toBe(PWA_ICON_192_PATH);
    expect(manifest.icons[1]?.src).toBe(PWA_ICON_512_PATH);
  });

  it("buildWebManifest uses router name when short enough", () => {
    const manifest = buildWebManifest("Garage");
    expect(manifest.name).toBe("Garage");
    expect(manifest.short_name).toBe("Garage");
  });

  it("installPwaManifest upserts head tags with manifest path", () => {
    document.head.replaceChildren();
    installPwaManifest("Balansun");
    expect(document.querySelector('link[rel="manifest"]')?.getAttribute("href")).toBe(
      PWA_MANIFEST_PATH,
    );
    expect(document.querySelector('meta[name="apple-mobile-web-app-capable"]')?.content).toBe("yes");
    expect(document.querySelector('link[rel="apple-touch-icon"]')?.getAttribute("href")).toBe(
      PWA_ICON_192_PATH,
    );
  });
});
