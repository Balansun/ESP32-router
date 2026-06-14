import { ensureUiFresh } from "./pwa/uiCacheRefresh";

/** Boot: one API fetch at a time; PWA manifest/SW install after main bootstrap. */
void (async () => {
  await ensureUiFresh();
  await import("./main");
})();
