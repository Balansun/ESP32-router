# Firmware build notes

**Field deployment:** flash from [GitHub Releases](https://github.com/Balansun/ESP32-router/releases), join Wi‑Fi, set HTTP API password if needed, OTA from nightly releases. **Build and CI:** see [CONTRIBUTING.md](../CONTRIBUTING.md).

## Stack

- **Primary build**: [PlatformIO](../platformio.ini) with `framework = arduino` and `src_dir = firmware`.
- **Arduino-ESP32 and ESP-IDF**: the Arduino core is built on Espressif’s ESP-IDF. You can call native IDF APIs from `.cpp` files where needed. A full **CMake-first project with Arduino only as an ESP-IDF component** is possible (see [Arduino as an ESP-IDF component](https://docs.espressif.com/projects/arduino-esp32/en/latest/esp-idf_component.html)) but is **not** the default layout here: it increases maintenance cost (toolchain alignment, `WebServer`/Arduino init ordering) without changing runtime behaviour.

## Production vs lab builds

| Env | RemoteDebug | Inject API |
|-----|-------------|------------|
| `wroom32` | off (`-DBALANSUN_REMOTE_DEBUG=0`) | off |
| `hil` | on | `POST /api/v1/sources/test/inject` |

## Meter packs (compile-time source selection)

**Core rule:** default (`BALANSUN_METER_PACK_FULL`) links **all** meter drivers; a specific pack links **exactly one** meter (+ shared transport helpers). Product **role** (`BALANSUN_ROLE_*`) is orthogonal and controls router vs meter-gateway behavior.

### Router vs meter-gateway

| Role | `surplus_regulation` | `triac` (advertised) | Meaning |
|------|---------------------|----------------------|---------|
| `METER_GATEWAY` (`*_meter`) | false | false | Measure only — no routing, no triac dimming |
| `METER_ROUTER` (`*_router`) | true | true (mirrors surplus) | Single-meter router — triac dimming is **always** present |
| `FULL_ROUTER` (`wroom32`, `hil`, …) | true | true | Full router — triac + optional `multi_action` |

Triac dimming is **not** an optional axis on router builds; consumers gate router/triac surfaces on `surplus_regulation` only.

| Axis | Macro | Default |
|------|-------|---------|
| Meter pack | `BALANSUN_METER_PACK` in [`balansun_meter_pack.h`](core/balansun_meter_pack.h) | `BALANSUN_METER_PACK_FULL` (all meters) |
| Product role | `BALANSUN_ROLE` in [`balansun_product_profile.h`](core/balansun_product_profile.h) | `BALANSUN_ROLE_FULL_ROUTER` |

Catalog: [`metering/meter_pack_manifest.json`](metering/meter_pack_manifest.json). Regenerate headers/envs:

```bash
python3 scripts/generate_meter_pack.py
python3 scripts/generate_meter_pack.py --check   # CI drift gate
```

| Env family | Role | Pack | Meters in binary |
|------------|------|------|------------------|
| `wroom32`, `hil`, `wrover`, `esp32s3_*` | `FULL_ROUTER` | `FULL` | all (+ `NotDef` on `hil`) |
| `{pack}_router` | `METER_ROUTER` | one pack | that meter only |
| `{pack}_meter` | `METER_GATEWAY` + `METER_ONLY_BUILD` | one pack | that meter only |
| `esp32_32u_jsy_router` | alias → `jsy_mk194_router` | `JSY_MK194` | `JsyMk194` only |
| `esp32_32u_jsy_meter` | alias → `jsy_mk194_meter` | `JSY_MK194` | `JsyMk194` only |
| `esp32c3_meter` | `METER_GATEWAY` | `JSY_MK194` | `JsyMk194` only |

Example single-meter flash (JSY-MK-194T gateway):

```bash
pio run -e jsy_mk194_meter
# or legacy alias:
pio run -e esp32_32u_jsy_meter
BALANSUN_UPLOAD_PORT=/dev/cu.usbserial-0001 pio run -e jsy_mk194_meter -t upload
```

```bash
pio run -e wroom32
pio run -e jsy_mk194_router -e jsy_mk194_meter -e linky_meter
python3 scripts/check_firmware_size.py
```

REST: `capabilities.product_profile`, `capabilities.firmware_capabilities.meter_pack`, `firmware_capabilities.meters[]` (see OpenAPI).

**Product profiles:** [`docs/PRODUCT_PROFILES.md`](../docs/PRODUCT_PROFILES.md) (roles, caps, PlatformIO envs, API errors, JSY 32U choice).

## Board-tier history retention

Daily energy history (`days_retained` / `days_capacity`) and RAM power rings are **compile-time** per PlatformIO env (see [`balansun_history_limits.h`](core/balansun_history_limits.h), [`balansun_pwr_hist_limits.h`](core/balansun_pwr_hist_limits.h)):

| Env | Daily retention | Power ring (5 min) | Default `GET /history/power` window |
|-----|-----------------|--------------------|-------------------------------------|
| `wroom32`, `hil`, `wroom32_*` | **7 days** | 288 slots (24 h) | `24h` |
| `wrover32`, `wrover` | **30 days** | 600 slots (48 h) | `48h` |
| `esp32s3`, `esp32s3_opi`, `esp32s3_ota` | **90 days** | 600 slots (48 h) | `48h` |

Build flags: `-DBALANSUN_HISTORY_DAYS_RETAINED=N`, `-DBALANSUN_PWR_HIST_5MN_SLOTS`, `-DBALANSUN_PWR_HIST_2S_SLOTS`. WROVER/S3 override WROOM defaults via `build_unflags` in [`platformio.ini`](../platformio.ini).

Runtime API caps (JSON size, `hist_max_points`) follow [`balansun_hw_profile`](core/balansun_hw_profile.cpp) tier (`constrained` / `standard` / `extended`). Device info exposes `capabilities.history_days_retained`, `history_power_window`, etc.

## Board-tier config backup / import

Config export/import caps are **runtime tier** (see [`balansun_hw_profile_logic.cpp`](core/balansun_hw_profile_logic.cpp)). `put_body_max` matches `config_export_json_cap` so a backup from the same device round-trips without HTTP 413.

| Tier | `config_export` / `put_body` | `full_config_export` | `pmqtt_bindings` in JSON | Sequenced parts |
|------|------------------------------|----------------------|--------------------------|-----------------|
| **constrained** (WROOM) | 20 KiB | false — sparse keys | No (metadata flags only) | `GET/PUT /system/backup/part/{network,config,actions}` (8 KiB each) |
| **standard** | 32 KiB | true — full config | Yes | Monolithic `/system/backup` only |
| **extended** | 49 KiB | true | Yes | Monolithic only |

Sparse export omits factory-default keys ([`balansun_config_defaults_logic.cpp`](core/balansun_config_defaults_logic.cpp)); import merges absent keys. Web UI downloads three files on constrained tier when `capabilities.backup_export_mode` is `parts`.

```bash
pio run -e wroom32    # production WROOM-32
pio run -e wrover32   # ESP32-WROVER-32 = wroom32 + BOARD_HAS_PSRAM (4 MB SPI RAM; `wrover` alias)
pio run -e esp32s3_opi  # ESP32-S3 with octal PSRAM
```

## Platform / Python

- The default **`wroom32`** environment pins **`platform = espressif32@6.4.0`** so the bundled **esptool** stays compatible with **Python 3.9** (common on macOS Command Line Tools). Newer Espressif32 platform releases may pull **esptool 5.x**, which expects **Python 3.10+**.
- To move to the latest `espressif32` platform, use a **Python 3.10+** virtualenv for PlatformIO and you may remove the `@6.4.0` pin after verifying `pio run` and OTA image size.

## Developer-only Wi‑Fi / MQTT defaults (optional)

Factory defaults are **empty** strings in [`balansun_globals.cpp`](core/balansun_globals.cpp). For a local lab image only, you can inject non-secret placeholders at compile time (never commit real credentials):

```text
build_flags =
  ${env:wroom32.build_flags}
  '-DBALANSUN_DEFAULT_WIFI_SSID="myssid"'
  '-DBALANSUN_DEFAULT_WIFI_PASSWORD="mypass"'
  '-DBALANSUN_DEFAULT_MQTT_USER="mqtt"'
  '-DBALANSUN_DEFAULT_MQTT_PASSWORD="mqttpass"'
```

Escape quotes as required by your shell; prefer a private `platformio.local.ini` (gitignored) for these lines.

**Never commit** real credentials, `web/.env.local`, or signed fleet JSON with password keys. With [pre-commit](https://pre-commit.com/) installed, Gitleaks runs on each commit (`.gitleaks.toml`; see [CONTRIBUTING.md](../CONTRIBUTING.md)).

## Layout (post-refactor)

| Area | Location |
|------|-----------|
| Entry | [`Balansun.ino`](Balansun.ino) → `balansun_setup()` / `balansun_loop()` |
| App / triac / temperature / energy | [`core/balansun_app.cpp`](core/balansun_app.cpp), [`core/balansun_triac_isr.cpp`](core/balansun_triac_isr.cpp) |
| Global state | [`core/balansun_globals.cpp`](core/balansun_globals.cpp) |
| REST `/api/v1` | [`api_v1_routes.cpp`](api_v1_routes.cpp) + [`api/`](api/) modules |
| Removed pre-v1 HTTP routes | [`http_server.cpp`](http_server.cpp) |
| MQTT HA | [`mqtt_ha.cpp`](mqtt_ha.cpp) |
| SPA routes | [`web_ui_routes.cpp`](web_ui_routes.cpp) |
| Metering | [`metering/*.cpp`](metering/) + [`core/balansun_source.cpp`](core/balansun_source.cpp) |

## PWA install (Android Chrome)

After `build_web.py`, firmware serves `/manifest.webmanifest`, `/pwa/icon-*.png`, and `/sw.js` from `pageHtmlPwaAssets.h`.

On-device check:

1. Open `http://<router-ip>/` — DevTools Application → manifest `display: fullscreen`, icons from `/pwa/`.
2. Remove any old home-screen shortcut, reinstall (banner **Install** or browser menu).
3. Launch **only from the home screen icon** — URL bar should be hidden.

Regenerate public PWA files: `cd web && npm run sync:brand` (runs `generate-pwa-icons.mjs`).

## Install country table

Regenerate after editing `scripts/generate-install-countries.mjs`:

```bash
node scripts/generate-install-countries.mjs
```

This updates `web/src/data/install-countries.ts` and `firmware/core/balansun_install_countries.{h,cpp}`.

## Contract checks

See [`firmware/test/golden/README.md`](test/golden/README.md) and [`scripts/check_firmware_golden.py`](../scripts/check_firmware_golden.py).

## Testing

Solar routing to the cumulus is safety- and comfort-critical: prefer **`./scripts/run_all_firmware_checks.sh`** before release, and run HIL on a bench ESP32 when changing regulation, metering, or triac code.

| Command | Purpose |
|---------|---------|
| `pio test -e native` | Host GoogleTest under [`test/native/`](test/native/) — regulation, meters, inject validation, **full-day replay** (`DayReplayLogic*`) |
| `pio test -e native -f DayReplayLogic` | Full-day replay smoke only |
| `rm -rf .pio && pio run -e wroom32` | After native tests, wipe `.pio` before ESP32 builds (native `platform=native` pollutes the tree; CI uses separate jobs) |
| `python3 scripts/check_openapi_drift.py` | [`openapi/balansun-v1.yaml`](../openapi/balansun-v1.yaml) vs firmware embed vs `web/test/fixtures/openapi-snapshot.json` |
| `python3 scripts/check_openapi_routes.py` | `api_v1_routes.cpp` (+ onNotFound sub-resources: actions, auth tokens) vs OpenAPI YAML paths |
| `python3 scripts/embed_openapi.py` | Regenerate PROGMEM OpenAPI in `api/api_v1_openapi.cpp` after editing YAML |
| `python3 scripts/sync_openapi_snapshot.py` | Regenerate web OpenAPI fixture from YAML |
| `python3 scripts/check_firmware_golden.py --suite firmware/test/golden/captures` | Per-route JSON key contracts |
| `./scripts/run_all_firmware_checks.sh` | CI-parity host gate: native, MQTT/OpenAPI/golden, web tests, `wroom32` build (+ HIL if `BALANSUN_HIL_URL` set) |
| `pio run -e hil` | Lab firmware with `POST /api/v1/sources/test/inject` |

HIL (hardware): see [`test/hil/README.md`](test/hil/README.md). Set `BALANSUN_HIL_URL`, optional `BALANSUN_HIL_PASSWORD`, then `pytest firmware/test/hil -q`. Inject API accepts optional `sim.wall_decihours` and `sim.temperature_c` for schedule replay without SNTP.

All firmware test assets are indexed in [`test/README.md`](test/README.md).
