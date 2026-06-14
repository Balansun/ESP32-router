# Contributing to Balansun Router (ESP32-router)

## Before you open a PR

Run the local CI-parity bundle (requires [ripgrep](https://github.com/BurntSushi/ripgrep), PlatformIO, Node 22):

```bash
./scripts/run_all_firmware_checks.sh
```

Fast path (skips native gcovr coverage gate):

```bash
./scripts/ci_host_checks.sh --skip-coverage
cd web && npm ci && npm run typecheck && npm run test:coverage
pio run -e wroom32 && ./scripts/check_firmware_flash_size.sh wroom32
```

Recommended [pre-commit](https://pre-commit.com/): `pre-commit install` then each commit runs [Gitleaks](https://github.com/gitleaks/gitleaks) (`.gitleaks.toml`), naming and asset checks, and web typecheck.

## CI on GitHub

| Workflow | What it gates |
|----------|----------------|
| [Firmware](.github/workflows/firmware.yml) | `ci_host_checks.sh`, coverage, builds, web typecheck + Vitest 95%, optional HIL |
| [Release](.github/workflows/release.yml) | Same host checks before release artifacts |

## Secrets and credentials

Never commit:

- `web/.env.local`, `platformio.local.ini`
- Real Wi‑Fi, MQTT, or API passwords in source or `build_flags`
- Fleet bundles containing password fields (blocked at runtime too)

Use gitignored `platformio.local.ini` for lab-only `BALANSUN_DEFAULT_*` compile flags — see [firmware/FIRMWARE_BUILD.md](firmware/FIRMWARE_BUILD.md).

## Documentation

Product and build docs live in this repository:

- [`firmware/FIRMWARE_BUILD.md`](firmware/FIRMWARE_BUILD.md) — flash, OTA, meter packs, release checklist
- [`docs/PRODUCT_PROFILES.md`](docs/PRODUCT_PROFILES.md) — PlatformIO envs and REST capabilities
- [`openapi/balansun-v1.yaml`](openapi/balansun-v1.yaml) — REST contract

Field-help markdown is generated from SPA locale tables:

- Summaries (in-app `?` popover): `web/src/i18n/locales/fieldHelp.{en,fr}.ts`
- Examples (optional export): `web/scripts/field-help-extras/{en,fr}.ts` — **add an entry for every new field-help key** in both languages

```bash
cd web && npx tsx scripts/generate-field-help-docs.ts
```

Output: `web/.field-help-docs/` (gitignored). Parity (keys, headings, example blocks) is checked by `web/test/field-help-docs-parity.test.ts`.

## Assets

- **Canonical brand SVGs:** `assets/brand/` only (inlined into firmware via `web/src/brand/brandAssets.ts`).
- **Do not commit** `web/public/` — regenerated on `npm run build` (`sync-brand-assets.mjs`, `generate-pwa-icons.mjs`).
- Wiring emulator icons belong in [Balansun-Emulator](https://github.com/Balansun/Balansun-Emulator), not this repo.
