"""HIL: profile role drives 403 capability_disabled on forbidden writes."""

from __future__ import annotations

import pytest

from profile_matrix import require_profile_row, unsupported_source_for_row
from hil_helpers import request_with_retry

pytestmark = pytest.mark.hil


def test_gateway_rejects_unsupported_source_put(hil_session, hil_base_url):
    row = require_profile_row()
    if row["role"] != "meter_gateway":
        pytest.skip(f"profile {row['product_profile']} is not meter_gateway")

    unsupported = unsupported_source_for_row(row)
    if unsupported is None:
        pytest.skip(f"no candidate unsupported source for {row['product_profile']}")

    r = request_with_retry(
        hil_session,
        "PATCH",
        f"{hil_base_url}/api/v1/config",
        json={"source": unsupported},
        timeout=20,
    )
    assert r.status_code == 403, r.text
    body = r.json()
    assert body.get("error") == "capability_disabled"
    assert body.get("missing_cap") == "unsupported_source"


def test_gateway_rejects_triac_override(hil_session, hil_base_url):
    row = require_profile_row()
    if row["role"] != "meter_gateway":
        pytest.skip(f"profile {row['product_profile']} is not meter_gateway")

    r = request_with_retry(
        hil_session,
        "POST",
        f"{hil_base_url}/api/v1/triac/override",
        json={"command": "auto"},
        timeout=20,
    )
    assert r.status_code == 403, r.text
    body = r.json()
    assert body.get("error") == "capability_disabled"
    assert body.get("missing_cap") == "surplus_regulation"


def test_router_allows_triac_override_auto(hil_session, hil_base_url):
    row = require_profile_row()
    if row["role"] not in ("meter_router", "full_router"):
        pytest.skip(f"profile {row['product_profile']} has no triac regulation")

    r = request_with_retry(
        hil_session,
        "POST",
        f"{hil_base_url}/api/v1/triac/override",
        json={"command": "auto"},
        timeout=20,
    )
    assert r.status_code == 200, r.text
    body = r.json()
    assert body.get("ok") is True
