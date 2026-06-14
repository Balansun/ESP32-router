from __future__ import annotations

import pytest


@pytest.fixture
def require_inject_api(hil_session, hil_base_url):
    r = hil_session.post(
        f"{hil_base_url}/api/v1/sources/test/inject",
        json={"house": {"active_import_w": 500, "active_export_w": 100}},
        timeout=15,
    )
    if r.status_code == 404:
        pytest.skip("HIL inject route not available (flash pio run -e hil)")
    return r


def test_inject_updates_measurements(hil_session, hil_base_url, require_inject_api):
    inj = require_inject_api
    assert inj.status_code == 200, inj.text
    m = hil_session.get(f"{hil_base_url}/api/v1/measurements", timeout=15).json()
    house = m["house"]
    assert house["active_import_w"] == 500
    assert house["active_export_w"] == 100
    assert house["grid_net_w"] == 400
    assert house["house_load_w"] == 500
    assert house["pv_production_w"] == 100


def test_inject_affects_state_triac_field(hil_session, hil_base_url, require_inject_api):
    require_inject_api.raise_for_status()
    st = hil_session.get(f"{hil_base_url}/api/v1/state", timeout=15).json()
    assert "triac_open_percent" in st
    assert isinstance(st["triac_open_percent"], (int, float))
