# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

## [0.1.0] - 2026-06-14

### Added

- ESP32 firmware for PV surplus routing: incomer metering (Linky TIC, JSY, Shelly, Enphase, MQTT/pMQTT, and more), triac regulation, configurable actions, vacation mode, and optional split meter/router deployment.
- Bilingual embedded web UI at `GET /`, REST `/api/v1`, OpenAPI contract, OTA from GitHub nightly releases, and MQTT with Home Assistant discovery.
- **pMQTT source:** setup wizard, JSON path picker, metric bindings, and live preview for MQTT measurement sources.
- **HTTP API protection:** LAN password, browser session sign-in, up to four permanent bearer tokens; Wi‑Fi AP setup stays reachable without sign-in; STA **More → Wi‑Fi** uses the same API session when protection is enabled.
- **MQTT JSON config** (schema_version 2), device automation triggers (`source_lost`, `regulation_hunting`, `vacation_ended`, `action_cap_hit`, …), and optional `triac_backoff_when_heater_idle` / `triac_off_when_source_stale` fail-safes.
- Optional [HACS](https://github.com/Balansun/HACS) Home Assistant integration (REST polling). Full MQTT entity set remains via discovery.
- OpenAPI snapshot (`openapi/balansun-v1.yaml`) and generated `pageHtmlApp.h` in this repository.
- GitHub releases are **nightly only** (`vX.Y.Z-nightly.<sha>` from main-branch CI); device OTA installs from the nightly channel.
- Reflashing after an EEPROM layout bump **factory-resets** persisted Wi‑Fi, MQTT, source, and actions — export a web backup before flashing. Load channels persist as JSON in EEPROM extension magic `0xE220`.

### Security

- When an HTTP API password is set, all `/api/v1/*` routes require `Authorization: Bearer <token>` (session or permanent token). Configure tokens under **More → API** in the web UI.
