import { embeddedUiVersion } from "./installManifest";

/** Register firmware-served service worker for installed PWA shell caching. */
export async function registerServiceWorker(): Promise<void> {
  if (!("serviceWorker" in navigator)) return;
  try {
    const reg = await navigator.serviceWorker.register("/sw.js", { scope: "/" });
    const version = embeddedUiVersion();
    reg.addEventListener("updatefound", () => {
      const worker = reg.installing;
      if (!worker) return;
      worker.addEventListener("statechange", () => {
        if (worker.state === "installed" && navigator.serviceWorker.controller) {
          worker.postMessage({ type: "SKIP_WAITING" });
        }
      });
    });
    if (version && version !== "dev") {
      await reg.update().catch(() => {});
    }
  } catch {
    /* SW may be unavailable on non-secure origins */
  }
}
