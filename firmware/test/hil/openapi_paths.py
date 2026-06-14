"""Safe GET paths from web OpenAPI fixture (shared with HIL smoke)."""

from __future__ import annotations

import json
from pathlib import Path

FIXTURE = Path(__file__).resolve().parents[3] / "web" / "test" / "fixtures" / "openapi-snapshot.json"

SKIP_PREFIXES = ()
SKIP_EXACT = set()


def load_openapi_paths() -> list[str]:
    doc = json.loads(FIXTURE.read_text(encoding="utf-8"))
    paths = []
    for path, methods in doc.get("paths", {}).items():
        if "get" not in methods:
            continue
        if "{index}" in path or "{index}" in path:
            continue
        paths.append(path)
    return sorted(paths)


SAFE_GET_PATHS = [
    "/api/v1/measurements",
    "/api/v1/tariff/tempo",
    "/api/v1/system",
    "/api/v1/system/audit",
    "/api/v1/device",
    "/api/v1/state",
    "/api/v1/telemetry/snapshot",
    "/api/v1/health",
    "/api/v1/public",
    "/api/v1/sources",
    "/api/v1/sources/brute_panel",
    "/api/v1/sources/diagnostics",
    "/api/v1/config",
    "/api/v1/actions",
    "/api/v1/actions/schema",
    "/api/v1/actions/config",
    "/api/v1/history/power",
    "/api/v1/history/energy/daily",
    "/api/v1/system/pins",
    "/api/v1/gpio",
    "/api/v1/pwm",
    "/api/v1/fleet/export",
    "/api/v1/wifi",
    "/api/v1/wifi/scan",
    "/api/v1/system/eeprom",
    "/api/v1/system/backup",
    "/api/v1/system/arduino-ota",
    "/api/v1/time",
    "/api/v1/openapi.json",
]
