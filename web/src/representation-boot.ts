/**
 * Dev-only entry: full SPA against the local mock API (no service worker cache).
 * Open via `npm run dev:representation` → http://127.0.0.1:5173/representation.html
 */
import { installPwaManifest } from "./pwa/installManifest";

installPwaManifest("Balansun (preview)");
void import("./main");
