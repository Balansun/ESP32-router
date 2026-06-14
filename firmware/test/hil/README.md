# Hardware-in-the-loop (HIL) tests

Pytest suite against a **real ESP32** on the bench (flash `hil` env, poll health, exercise REST).

## Setup

1. Flash: `pio run -e hil -t upload` (from repo root).
2. Set environment variables:
   - `BALANSUN_HIL_URL` — e.g. `http://192.168.1.50`
   - `BALANSUN_HIL_PASSWORD` — HTTP API password when auth is enabled (preferred on the lab bench)
   - `BALANSUN_HIL_USER` — optional (default `admin`)
   - `BALANSUN_API_BEARER_TOKEN` — optional automation PAT; if set but revoked on the device, pytest falls back to password login when `BALANSUN_HIL_PASSWORD` is set. **Unset** a stale token to avoid mass 401s: `unset BALANSUN_API_BEARER_TOKEN`

## Run locally

```bash
export BALANSUN_HIL_URL=http://192.168.2.159
export BALANSUN_HIL_PASSWORD='your-api-password'
unset BALANSUN_API_BEARER_TOKEN
pytest firmware/test/hil -q
```

Pin map contract tests use `GET/PUT /api/v1/system/pins` (not sparse `GET /config`). GPIO map in the UI also loads `/system/pins`.

Destructive EEPROM round-trip: `BALANSUN_HIL_EEPROM_ROUNDTRIP=1 pytest firmware/test/hil/test_eeprom_roundtrip.py`

Set `BALANSUN_HIL_REQUIRE_REGULATION=1` to fail instead of skip when the bench device is not regulating.

## CI

Job `firmware-hil` in [`.github/workflows/firmware.yml`](../../.github/workflows/firmware.yml) runs **only when** repository secret `BALANSUN_HIL_URL` is set (checked by job `hil-gate`; self-hosted runner label `esp32-hil`). Without that secret, host jobs (`firmware-native`, `firmware-build`, `web-contract`) still run on every push/PR.

When the HIL job runs, `BALANSUN_HIL_REQUIRE_REGULATION=1` is set so regulation tests **fail** instead of skipping when action 0 is not in AUTO.

Optional secrets: `BALANSUN_HIL_USER`, `BALANSUN_HIL_PASSWORD`.

## Host gate

[`scripts/run_all_firmware_checks.sh`](../../scripts/run_all_firmware_checks.sh) runs native tests, [`scripts/check_firmware_golden.py`](../../scripts/check_firmware_golden.py) (same `required_*_keys.json` as `test_api_smoke`), MQTT/OpenAPI checks, web Vitest, and `wroom32` build. Set `BALANSUN_HIL_URL` to append HIL pytest.

Quick API smoke on the bench (or mock): `BALANSUN_HIL_URL=http://192.168.x.x ./scripts/field_smoke_api.sh`

## Dual-profile matrix (AP + standard)

| Env | Role |
|-----|------|
| `hil_test_ap` | Forces setup AP at `http://192.168.4.1` (`BALANSUN_HIL_FORCE_AP`) |
| `hil_test_standard` | Embeds lab Wi-Fi at compile time (`BALANSUN_LAB_WIFI_*`) |

Restore home config before tests:

```bash
export BALANSUN_HIL_URL=http://192.168.2.159
export BALANSUN_API_BEARER_TOKEN=...
python3 scripts/apply_home_config_fixture.py
```

Full matrix (flash both profiles, route + limit coverage):

```bash
export BALANSUN_HIL_URL=http://192.168.2.159
export BALANSUN_HIL_AP_URL=http://192.168.4.1
export BALANSUN_LAB_WIFI_SSID='GrimmChester'
export BALANSUN_LAB_WIFI_PASSWORD='...'
export BALANSUN_API_BEARER_TOKEN=...
export BALANSUN_UPLOAD_PORT=192.168.2.159
./scripts/run_hil_device_matrix.sh
```

Host drift check:

```bash
python3 scripts/check_limit_catalog_drift.py
python3 scripts/generate_profile_test_matrix.py --check
python3 scripts/check_profile_wire_drift.py
```

## Profile / board variant matrix (USB bench)

Generated matrix: [`firmware/test/golden/profile_test_matrix.json`](../golden/profile_test_matrix.json) (25 variants: 11 packs × router/gateway + ESP32-32U aliases + `hil` full router).

Per-flash HIL (sequential USB uploads on one board):

```bash
export BALANSUN_HIL_URL=http://192.168.2.159
export BALANSUN_UPLOAD_PORT=/dev/cu.usbserial-0001
export BALANSUN_API_BEARER_TOKEN=...
# Optional filter for dev loops:
# export BALANSUN_HIL_MATRIX_PACK=jsy_mk194
# export BALANSUN_HIL_MATRIX_ROLE=meter_gateway
./scripts/run_hil_profile_matrix.sh
```

Each flash sets `BALANSUN_HIL_EXPECT_PROFILE` and runs:

- `test_device_profile_matrix.py` — caps match matrix row
- `test_profile_caps.py` — role-based 403 checks
- `test_pwm_config_caps.py` — PWM validation on routers
- `test_config_settings_roundtrip.py` — PATCH then GET (`router_name`, PWM) verifies persisted values
- `test_safety_lockout.py` — health `safety_lockout` contract on all profiles; routing mutator lockout on routers (uses `POST /api/v1/health/self-test/simulate` on `hil` builds)
- `test_inject_*.py` / regulation — **only** after `hil` (full_router) flash

Full local gate (host + web + compile all envs + optional matrix):

```bash
./scripts/run_intensive_verification.sh
```

Restores `esp32_32u_jsy_router` at end unless `BALANSUN_HIL_SKIP_RESTORE=1`.
