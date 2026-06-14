"""100% safe GET route coverage on the connected HIL device."""
from __future__ import annotations

import pytest
import requests

from hil_helpers import get_with_retry, poll_wifi_scan, assert_get_ok_or_telemetry_not_ready
from openapi_paths import SAFE_GET_PATHS

PUBLIC_PATHS = {"/api/v1/public"}


@pytest.mark.parametrize("path", SAFE_GET_PATHS)
def test_safe_get_route(hil_session, hil_base_url, path):
    if path == "/api/v1/wifi/scan":
        poll_wifi_scan(hil_session, hil_base_url)
        return
    r = get_with_retry(hil_session, f"{hil_base_url}{path}", timeout=25)
    assert_get_ok_or_telemetry_not_ready(r, path)
    if r.status_code == 200:
        r.json()


def test_public_without_auth(hil_base_url):
    r = requests.get(f"{hil_base_url.rstrip('/')}/api/v1/public", timeout=10)
    assert r.status_code == 200
    doc = r.json()
    assert "wifi" in doc


def test_openapi_path_index(hil_session, hil_base_url):
    r = get_with_retry(hil_session, f"{hil_base_url}/api/v1/openapi.json", timeout=15, attempts=5)
    assert r.status_code == 200
    paths = r.json().get("paths", {})
    for core in ("/api/v1/device", "/api/v1/measurements", "/api/v1/health"):
        assert core in paths


def test_wifi_profile_shape(hil_session, hil_base_url, hil_profile):
    r = get_with_retry(hil_session, f"{hil_base_url}/api/v1/wifi", timeout=15)
    assert r.status_code == 200
    wifi = r.json()
    if hil_profile == "ap":
        assert wifi.get("setup_ap") is True
        assert wifi.get("mode") == "ap"
    else:
        assert wifi.get("mode") in ("sta", "ap")
