"""HIL: commissioning self-test safety lockout blocks routing mutators."""

from __future__ import annotations

import json
from pathlib import Path

import pytest

from hil_helpers import (
    assert_safety_lockout_error,
    device_surplus_regulation,
    get_with_retry,
    read_state,
    request_with_retry,
    require_simulate_api,
    restore_passing_self_test,
    simulate_self_test,
    wait_self_test_idle,
)

pytestmark = pytest.mark.hil

GOLDEN_DIR = Path(__file__).resolve().parents[1] / "golden"


def _require_router_surplus(hil_session, hil_base_url) -> None:
    if not device_surplus_regulation(hil_session, hil_base_url):
        pytest.skip("device profile has no surplus routing (meter gateway)")


def test_health_safety_lockout_contract(hil_session, hil_base_url):
    r = get_with_retry(hil_session, f"{hil_base_url}/api/v1/health", timeout=20)
    r.raise_for_status()
    body = r.json()

    health_spec = json.loads(
        (GOLDEN_DIR / "required_health_keys.json").read_text(encoding="utf-8")
    )
    for key in health_spec["required_keys"]:
        assert key in body, f"missing health.{key}"

    lockout = body.get("safety_lockout") or {}
    assert isinstance(lockout.get("active"), bool), lockout
    assert isinstance(lockout.get("reasons"), list), lockout

    if not device_surplus_regulation(hil_session, hil_base_url):
        assert lockout.get("active") is not True
        return

    st_spec = json.loads(
        (GOLDEN_DIR / "required_health_self_test_keys.json").read_text(encoding="utf-8")
    )
    nested = body.get("self_test") or {}
    for key in st_spec["required_keys"]:
        assert key in nested, f"missing health.self_test.{key}"


def test_safety_lockout_blocks_routing_mutators(hil_session, hil_base_url):
    _require_router_surplus(hil_session, hil_base_url)
    require_simulate_api(hil_session, hil_base_url)
    restore_passing_self_test(hil_session, hil_base_url)

    fail = simulate_self_test(
        hil_session,
        hil_base_url,
        zc_ok=False,
        triac_ok=True,
        source_ok=True,
    )
    fail.raise_for_status()

    health = get_with_retry(hil_session, f"{hil_base_url}/api/v1/health", timeout=20).json()
    lockout = health.get("safety_lockout") or {}
    assert lockout.get("active") is True, health
    assert "zc_failed" in (lockout.get("reasons") or []), lockout

    nested = health.get("self_test") or {}
    assert nested.get("safety_lockout_active") is True
    assert nested.get("severity", {}).get("zc") == "critical"

    st = read_state(hil_session, hil_base_url)
    suspend = (st.get("status") or {}).get("output_suspend") or {}
    assert suspend.get("active") is True
    assert suspend.get("reason") == "safety_lockout"

    triac = request_with_retry(
        hil_session,
        "POST",
        f"{hil_base_url}/api/v1/triac/override",
        json={"command": "50"},
        timeout=20,
    )
    assert_safety_lockout_error(triac)

    pwm = request_with_retry(
        hil_session,
        "PATCH",
        f"{hil_base_url}/api/v1/config",
        json={"pwm_mode": "independent"},
        timeout=20,
    )
    assert_safety_lockout_error(pwm)

    restore_passing_self_test(hil_session, hil_base_url)
    health2 = get_with_retry(hil_session, f"{hil_base_url}/api/v1/health", timeout=20).json()
    assert health2.get("safety_lockout", {}).get("active") is not True


def test_action_override_blocked_under_lockout(hil_session, hil_base_url):
    _require_router_surplus(hil_session, hil_base_url)
    require_simulate_api(hil_session, hil_base_url)
    simulate_self_test(
        hil_session,
        hil_base_url,
        zc_ok=True,
        triac_ok=False,
        source_ok=True,
    ).raise_for_status()

    r = request_with_retry(
        hil_session,
        "POST",
        f"{hil_base_url}/api/v1/actions/0/override",
        json={"state": "on"},
        timeout=20,
    )
    assert_safety_lockout_error(r)

    restore_passing_self_test(hil_session, hil_base_url)


def test_self_test_run_allowed_under_lockout(hil_session, hil_base_url):
    _require_router_surplus(hil_session, hil_base_url)
    require_simulate_api(hil_session, hil_base_url)
    simulate_self_test(
        hil_session,
        hil_base_url,
        zc_ok=False,
        triac_ok=True,
        source_ok=True,
    ).raise_for_status()

    r = request_with_retry(
        hil_session,
        "POST",
        f"{hil_base_url}/api/v1/health/self-test/run",
        timeout=20,
    )
    assert r.status_code == 200, r.text[:300]

    wait_self_test_idle(hil_session, hil_base_url)
    restore_passing_self_test(hil_session, hil_base_url)


def test_inject_blocked_under_lockout(hil_session, hil_base_url):
    _require_router_surplus(hil_session, hil_base_url)
    require_simulate_api(hil_session, hil_base_url)
    simulate_self_test(
        hil_session,
        hil_base_url,
        zc_ok=False,
        triac_ok=True,
        source_ok=True,
    ).raise_for_status()

    health = get_with_retry(hil_session, f"{hil_base_url}/api/v1/health", timeout=20).json()
    assert (health.get("safety_lockout") or {}).get("active") is True, health

    r = request_with_retry(
        hil_session,
        "POST",
        f"{hil_base_url}/api/v1/sources/test/inject",
        json={"house": {"active_import_w": 0, "active_export_w": 2000}},
        timeout=20,
    )
    if r.status_code == 404:
        pytest.skip("inject route not available")
    assert_safety_lockout_error(r)

    restore_passing_self_test(hil_session, hil_base_url)
