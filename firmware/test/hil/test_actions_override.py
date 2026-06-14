from __future__ import annotations

import time

import pytest

from hil_helpers import inject_house, read_state, require_inject_api, triac_open_percent


def test_override_off_then_clear(hil_session, hil_base_url):
    require_inject_api(hil_session, hil_base_url)
    inject_house(hil_session, hil_base_url, 80, 3000)
    time.sleep(0.5)
    r = hil_session.post(
        f"{hil_base_url}/api/v1/actions/0/override",
        json={"state": "off"},
        timeout=15,
    )
    if r.status_code == 404:
        pytest.skip("override route unavailable")
    r.raise_for_status()
    time.sleep(0.5)
    assert triac_open_percent(hil_session, hil_base_url) == 0.0
    r = hil_session.post(f"{hil_base_url}/api/v1/actions/0/override/clear", timeout=15)
    r.raise_for_status()


def test_override_triac_fixed(hil_session, hil_base_url):
    require_inject_api(hil_session, hil_base_url)
    r = hil_session.post(
        f"{hil_base_url}/api/v1/actions/0/override",
        json={"state": "triac_fixed", "triac_open_percent": 35},
        timeout=15,
    )
    if r.status_code == 404:
        pytest.skip("override route unavailable")
    r.raise_for_status()
    time.sleep(0.5)
    pct = triac_open_percent(hil_session, hil_base_url)
    assert 30 <= pct <= 40
    hil_session.post(f"{hil_base_url}/api/v1/actions/0/override/clear", timeout=15)
    st = read_state(hil_session, hil_base_url)
    assert "triac_open_percent" in st
