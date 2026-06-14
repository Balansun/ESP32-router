# Golden fixtures (API contract smoke checks)

These files support **host-side** checks that JSON responses from a device (or mock) still expose the expected top-level keys for critical routes.

- `required_*_keys.json` — per-route contracts used by `scripts/check_firmware_golden.py`.
- `captures/` — example JSON payloads validated with `python3 scripts/check_firmware_golden.py --suite firmware/test/golden/captures`.
- `mqtt/topic_shapes.json` — MQTT topic strings checked by native `test_mqtt_ha_logic.cpp`.
- `mqtt/discovery_sensor_pw.json`, `mqtt/state_payload_shape.json` — MQTT payload contracts.
- `regulation/inject_export_state.json` — example triac band after export inject.
- `fixtures/days/` — synthetic 24 h profiles (`summer_weekday.json`, `winter_weekday.json`, `cloudy_day.json`) replayed by native `day_replay_logic` and HIL `test_fullday_replay.py`.
- Capture real payloads with `curl` / the web UI snapshot tooling, then refresh captures or run the script on a single file.

## Surplus routing (cumulus)

The product goal is to route **excess solar** to the water heater (triac / cumulus). Tests that guard this behaviour:

| Layer | What it checks |
|-------|----------------|
| Native (`pio test -e native`) | `balansun_regulation_logic` + schedule wiring: export net power opens triac, import closes, caps, overrides, hot-tank temperature block |
| Golden `state.json` | `triac_open_percent` present on `/api/v1/state` |
| HIL (`firmware/test/hil/test_surplus_routing.py`) | Injected import vs export profiles move `triac_open_percent` on a flashed `hil` build (skips if action 0 is not regulating) |

EEPROM layout compatibility is enforced by firmware revision discipline (`kEepromLayoutInit` in [`balansun_board.h`](../core/balansun_board.h) and the layout in [`metering/storage_eeprom.cpp`](../metering/storage_eeprom.cpp)); automated on-device round-trips belong in HIL / hardware-in-the-loop runs, not in this lightweight host script.
