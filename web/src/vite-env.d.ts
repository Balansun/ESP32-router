/// <reference types="vite/client" />

interface ImportMetaEnv {
  readonly VITE_GITHUB_OWNER?: string;
  readonly VITE_GITHUB_REPO?: string;
  /** Set by build_web.py for PROGMEM firmware embed — trims maintainer-only UI. */
  readonly VITE_FIRMWARE_BUNDLE?: string;
  /** Firmware Version macro baked into the SPA at build time (cache bust). */
  readonly VITE_FIRMWARE_VERSION?: string;
  /** Public documentation site origin (no trailing slash), e.g. https://balansun.clouded.fr */
  readonly VITE_DOCS_SITE_ORIGIN?: string;
}

interface ImportMeta {
  readonly env: ImportMetaEnv;
}
