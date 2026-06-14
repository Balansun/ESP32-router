from __future__ import annotations

import pytest
import requests

from hil_helpers import wait_hil_health
from limit_helpers import (
    http_auth_enabled,
    inject_route_available,
    load_limit_catalog,
    run_over_limit,
    run_query_clamp,
)

CATALOG = load_limit_catalog()
BODY_ENTRIES = [e for e in CATALOG if e.get("limit_type") == "body_bytes"]
QUERY_ENTRIES = [e for e in CATALOG if e.get("limit_type") == "query_clamp"]
VALIDATION_ENTRIES = [e for e in CATALOG if e.get("limit_type") == "validation"]


@pytest.fixture(scope="session")
def auth_enabled(hil_session, hil_base_url) -> bool:
    return http_auth_enabled(hil_session, hil_base_url)


@pytest.fixture(scope="session")
def inject_available(hil_session, hil_base_url) -> bool:
    return inject_route_available(hil_session, hil_base_url)


@pytest.mark.parametrize("entry", BODY_ENTRIES, ids=lambda e: e["id"])
def test_body_over_limit_rejected(
    hil_session,
    hil_base_url,
    auth_enabled,
    inject_available,
    entry,
):
    if entry.get("hil_only") and not inject_available:
        pytest.skip("HIL inject route not available")
    if entry.get("requires_auth_enabled") and not auth_enabled:
        pytest.skip("HTTP API auth disabled")
    try:
        r = run_over_limit(hil_session, hil_base_url, entry)
    except requests.ConnectionError:
        # ESP32 WebServer may drop the socket instead of returning 413 on huge bodies.
        wait_hil_health(hil_session, hil_base_url)
        return
    assert r.status_code == entry["over_status"], r.text[:300]
    if entry.get("over_error"):
        body = r.json()
        assert body.get("error") == entry["over_error"]


@pytest.mark.parametrize("entry", QUERY_ENTRIES, ids=lambda e: e["id"])
def test_query_over_limit_clamped(hil_session, hil_base_url, entry):
    r = run_query_clamp(hil_session, hil_base_url, entry)
    assert r.status_code == 200, r.text[:300]
    payload = r.json()
    requested = int(entry["max"]) + 1
    actual = payload.get(entry["expect_field"])
    assert actual is not None
    assert actual < requested
    assert actual <= int(entry["max"])


@pytest.mark.parametrize("entry", VALIDATION_ENTRIES, ids=lambda e: e["id"])
def test_validation_limit(hil_session, hil_base_url, entry):
    url = f"{hil_base_url.rstrip('/')}{entry['path']}"
    r = hil_session.request(entry["method"], url, json=entry["payload"], timeout=15)
    assert r.status_code == entry["expect_status"], r.text[:300]
    assert r.json().get("error") == entry["expect_error"]
