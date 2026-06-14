"""HIL: device caps must match profile_test_matrix row for flashed firmware."""

from __future__ import annotations

import pytest

from profile_matrix import assert_device_caps, fetch_device, require_profile_row
from hil_helpers import get_with_retry

pytestmark = pytest.mark.hil


def test_device_caps_match_matrix(hil_session, hil_base_url):
    row = require_profile_row()
    device = fetch_device(hil_session, hil_base_url)
    assert_device_caps(device, row)


def test_health_reports_product_profile(hil_session, hil_base_url):
    row = require_profile_row()
    r = get_with_retry(hil_session, f"{hil_base_url}/api/v1/health", timeout=20, attempts=5)
    r.raise_for_status()
    body = r.json()
    assert body.get("product_profile") == row["product_profile"]


def test_openapi_index_available(hil_session, hil_base_url):
    require_profile_row()
    r = get_with_retry(hil_session, f"{hil_base_url}/api/v1/openapi.json", timeout=20, attempts=5)
    r.raise_for_status()
    body = r.json()
    assert isinstance(body.get("paths"), dict)
    assert "/api/v1/device" in body["paths"]
