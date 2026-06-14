from __future__ import annotations

import json
from pathlib import Path

import pytest

from hil_helpers import get_with_retry, poll_wifi_scan, assert_get_ok_or_telemetry_not_ready
from openapi_paths import SAFE_GET_PATHS

GOLDEN_DIR = Path(__file__).resolve().parents[1] / "golden"

ROUTE_SPECS = {
    "/api/v1/device": "required_device_keys.json",
    "/api/v1/measurements": "required_measurements_keys.json",
    "/api/v1/config": "required_config_keys.json",
    "/api/v1/state": "required_state_keys.json",
    "/api/v1/telemetry/snapshot": "required_telemetry_snapshot_keys.json",
    "/api/v1/actions": "required_actions_keys.json",
    "/api/v1/actions/config": "required_actions_config_keys.json",
    "/api/v1/actions/schema": "required_actions_schema_keys.json",
    "/api/v1/sources": "required_sources_keys.json",
    "/api/v1/sources/diagnostics": "required_sources_diagnostics_keys.json",
    "/api/v1/health": "required_health_keys.json",
    "/api/v1/system": "required_system_keys.json",
    "/api/v1/history/energy/daily": "required_history_energy_daily_keys.json",
}


@ pytest.mark.parametrize("path", SAFE_GET_PATHS)
def test_safe_get_returns_json(hil_session, hil_base_url, path):
    if path == "/api/v1/wifi/scan":
        poll_wifi_scan(hil_session, hil_base_url)
        return
    timeout = 45 if path == "/api/v1/system/backup" else 20
    r = get_with_retry(hil_session, f"{hil_base_url}{path}", timeout=timeout)
    assert_get_ok_or_telemetry_not_ready(r, path)
    if r.status_code == 200:
        r.json()


@pytest.mark.parametrize("path,spec_file", list(ROUTE_SPECS.items()))
def test_get_route_contract(hil_session, hil_base_url, path, spec_file):
    r = get_with_retry(hil_session, f"{hil_base_url}{path}", timeout=15)
    assert_get_ok_or_telemetry_not_ready(r, path)
    if r.status_code != 200:
        return
    payload = r.json()
    spec = json.loads((GOLDEN_DIR / spec_file).read_text(encoding="utf-8"))
    for key in spec["required_keys"]:
        assert key in payload, f"missing {key} on {path}"


def test_openapi_lists_core_paths(openapi_paths):
    for p in ["/api/v1/device", "/api/v1/measurements", "/api/v1/health"]:
        assert p in openapi_paths
