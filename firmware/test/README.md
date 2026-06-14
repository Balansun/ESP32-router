# Firmware tests

All firmware-side test assets live under this directory.

| Path | Role |
|------|------|
| [`native/`](native/) | Host GoogleTest (`pio test -e native`) |
| [`native/stubs/`](native/stubs/) | Arduino stubs for native builds |
| [`fixtures/`](fixtures/) | Meter frames, inject payloads, regulation steps, **full-day** profiles |
| [`golden/`](golden/) | API JSON contracts + reference replay outputs |
| [`hil/`](hil/) | Pytest against a flashed ESP32 (`pio run -e hil`) |

## Commands (repo root)

```bash
pio test -e native
pio test -e native -f DayReplayLogic    # full-day replay smoke only
python3 scripts/check_firmware_golden.py --suite firmware/test/golden/captures
python3 scripts/check_openapi_drift.py
./scripts/run_all_firmware_checks.sh   # full pre-release gate (see firmware/test/hil/README.md)

# Hardware bench
export BALANSUN_HIL_URL=http://192.168.x.x
pytest firmware/test/hil -q
```

Surplus routing to the cumulus is covered by native `balansun_regulation_logic` / `day_replay_logic` tests and HIL inject scenarios.
