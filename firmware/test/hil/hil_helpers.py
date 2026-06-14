"""Shared helpers for HIL pytest modules."""

from __future__ import annotations

import json
import os
import time
from pathlib import Path
from typing import Any

import pytest
import requests

FIXTURES = Path(__file__).resolve().parents[1] / "fixtures"

# Routes that return 503 not_ready until first valid meter poll (configured lifecycle).
TELEMETRY_GET_PATHS = frozenset(
    {
        "/api/v1/measurements",
        "/api/v1/state",
        "/api/v1/telemetry/snapshot",
        "/api/v1/sources/diagnostics",
    }
)


def is_telemetry_not_ready_response(response: requests.Response) -> bool:
    if response.status_code != 503:
        return False
    try:
        body = response.json()
    except (json.JSONDecodeError, ValueError):
        return False
    return body.get("error") == "not_ready" and body.get("missing_cap") == "telemetry"


def assert_get_ok_or_telemetry_not_ready(response: requests.Response, path: str) -> None:
    if path in TELEMETRY_GET_PATHS and is_telemetry_not_ready_response(response):
        return
    assert response.status_code == 200, f"{path}: {response.text[:200]}"


def wait_hil_health(
    session: requests.Session,
    base_url: str,
    *,
    timeout_s: float = 90,
    poll_s: float = 2,
) -> None:
    """Wait until GET /health returns 200 (after device reboot or connection drop)."""
    deadline = time.time() + timeout_s
    last_exc: BaseException | None = None
    while time.time() < deadline:
        try:
            r = session.get(f"{base_url.rstrip('/')}/api/v1/health", timeout=15)
            if r.status_code == 200:
                return
        except requests.RequestException as exc:
            last_exc = exc
        time.sleep(poll_s)
    if last_exc is not None:
        raise last_exc
    pytest.fail(f"device at {base_url} did not recover within {timeout_s}s")


def get_measurements_or_skip_not_ready(
    session: requests.Session,
    base_url: str,
    *,
    timeout: float = 15,
) -> dict[str, Any]:
    r = get_with_retry(session, f"{base_url.rstrip('/')}/api/v1/measurements", timeout=timeout)
    if is_telemetry_not_ready_response(r):
        pytest.skip("telemetry not ready (no live meter on bench)")
    assert r.status_code == 200, r.text[:300]
    return r.json()


def request_with_retry(
    session: requests.Session,
    method: str,
    url: str,
    *,
    timeout: float = 20,
    attempts: int = 3,
    backoff_s: float = 2,
    **kwargs: Any,
) -> requests.Response:
    """HTTP request with retries on transient errors or 5xx."""
    last_exc: BaseException | None = None
    for attempt in range(attempts):
        try:
            r = session.request(method, url, timeout=timeout, **kwargs)
            if r.status_code < 500:
                return r
            if attempt + 1 < attempts:
                time.sleep(backoff_s)
                continue
            return r
        except (
            requests.Timeout,
            requests.ConnectionError,
            requests.exceptions.ChunkedEncodingError,
        ) as exc:
            last_exc = exc
            if attempt + 1 < attempts:
                time.sleep(backoff_s)
    if last_exc is not None:
        base = url.rsplit("/api/v1/", 1)[0] if "/api/v1/" in url else url.rstrip("/")
        wait_hil_health(session, base, timeout_s=45)
        return session.request(method, url, timeout=timeout, **kwargs)
    raise RuntimeError("request_with_retry: no response")


def get_with_retry(
    session: requests.Session,
    url: str,
    *,
    timeout: float = 20,
    attempts: int = 3,
    backoff_s: float = 2,
) -> requests.Response:
    """GET with retries on transient timeouts or connection errors."""
    last_exc: BaseException | None = None
    for attempt in range(attempts):
        try:
            r = session.get(url, timeout=timeout)
            if r.status_code == 200:
                if "json" in (r.headers.get("Content-Type") or "").lower():
                    try:
                        r.json()
                    except requests.exceptions.JSONDecodeError:
                        if attempt + 1 < attempts:
                            time.sleep(backoff_s)
                            continue
                return r
            if attempt + 1 < attempts and r.status_code >= 500:
                time.sleep(backoff_s)
                continue
            return r
        except (
            requests.Timeout,
            requests.ConnectionError,
            requests.exceptions.ChunkedEncodingError,
            requests.exceptions.JSONDecodeError,
        ) as exc:
            last_exc = exc
            if attempt + 1 < attempts:
                time.sleep(backoff_s)
    if last_exc is not None:
        base = url.rsplit("/api/v1/", 1)[0] if "/api/v1/" in url else url.rstrip("/")
        wait_hil_health(session, base, timeout_s=45)
        return session.get(url, timeout=timeout)
    raise RuntimeError("get_with_retry: no response")


def poll_wifi_scan(
    session: requests.Session,
    base_url: str,
    *,
    max_wait_s: float = 30,
    poll_s: float = 0.5,
    timeout: float = 20,
    request_attempts: int = 3,
) -> dict[str, Any]:
    """Poll GET /api/v1/wifi/scan until scan completes (HTTP 200, scanning false)."""
    url = f"{base_url.rstrip('/')}/api/v1/wifi/scan"
    deadline = time.time() + max_wait_s
    while time.time() < deadline:
        r: requests.Response | None = None
        last_exc: BaseException | None = None
        for attempt in range(request_attempts):
            try:
                r = session.get(url, timeout=timeout)
                last_exc = None
                break
            except (requests.Timeout, requests.ConnectionError) as exc:
                last_exc = exc
                if attempt + 1 < request_attempts:
                    time.sleep(poll_s)
        if last_exc is not None:
            raise last_exc
        assert r is not None
        if r.status_code not in (200, 202):
            r.raise_for_status()
        data = r.json()
        if r.status_code == 200 and not data.get("scanning", False):
            return data
        time.sleep(poll_s)
    raise TimeoutError(f"Wi-Fi scan did not complete within {max_wait_s}s")


def inject_payload(
    session: requests.Session,
    base_url: str,
    payload: dict[str, Any],
    *,
    timeout: float = 15,
) -> requests.Response:
    return session.post(f"{base_url.rstrip('/')}/api/v1/sources/test/inject", json=payload, timeout=timeout)


def inject_house(
    session: requests.Session,
    base_url: str,
    active_import_w: int,
    active_export_w: int,
    *,
    wall_decihours: int | None = None,
    temperature_c: float | None = None,
) -> requests.Response:
    payload: dict[str, Any] = {
        "house": {
            "active_import_w": active_import_w,
            "active_export_w": active_export_w,
        }
    }
    if wall_decihours is not None or temperature_c is not None:
        sim: dict[str, Any] = {}
        if wall_decihours is not None:
            sim["wall_decihours"] = wall_decihours
        if temperature_c is not None:
            sim["temperature_c"] = temperature_c
        payload["sim"] = sim
    return inject_payload(session, base_url, payload)


def read_state(session: requests.Session, base_url: str) -> dict[str, Any]:
    r = get_with_retry(session, f"{base_url.rstrip('/')}/api/v1/state", timeout=15)
    if is_telemetry_not_ready_response(r):
        pytest.skip("telemetry not ready (no live meter on bench)")
    r.raise_for_status()
    return r.json()


def triac_open_percent(session: requests.Session, base_url: str) -> float:
    return float(read_state(session, base_url)["triac_open_percent"])


def wait_triac_stable(
    session: requests.Session,
    base_url: str,
    *,
    timeout_s: float = 3.0,
    poll_s: float = 0.25,
    tolerance: float = 1.0,
) -> float:
    deadline = time.time() + timeout_s
    last = triac_open_percent(session, base_url)
    while time.time() < deadline:
        time.sleep(poll_s)
        cur = triac_open_percent(session, base_url)
        if abs(cur - last) <= tolerance:
            return cur
        last = cur
    return last


def ensure_hil_action0_regulating(session: requests.Session, base_url: str) -> None:
    """Provision triac action 0 when the bench EEPROM has no routing actions."""
    r = session.get(f"{base_url.rstrip('/')}/api/v1/actions/config", timeout=15)
    r.raise_for_status()
    cfg = r.json()
    if int(cfg.get("nb_actions") or 0) > 0:
        return
    payload = load_json_fixture("hil", "action0_regulating.json")
    put = session.put(
        f"{base_url.rstrip('/')}/api/v1/actions/config",
        json=payload,
        timeout=30,
    )
    assert put.status_code == 200, put.text[:500]


def require_inject_api(session: requests.Session, base_url: str) -> None:
    try:
        r = inject_house(session, base_url, 0, 0)
    except requests.ConnectionError:
        wait_hil_health(session, base_url)
        pytest.skip("device unreachable for inject probe")
    if r.status_code == 404:
        pytest.skip("HIL inject route not available (flash pio run -e hil)")
    ensure_hil_action0_regulating(session, base_url)


def simulate_self_test(
    session: requests.Session,
    base_url: str,
    *,
    zc_ok: bool,
    triac_ok: bool,
    source_ok: bool,
) -> requests.Response:
    return request_with_retry(
        session,
        "POST",
        f"{base_url.rstrip('/')}/api/v1/health/self-test/simulate",
        json={"zc_ok": zc_ok, "triac_ok": triac_ok, "source_ok": source_ok},
        timeout=20,
    )


def simulate_api_available(session: requests.Session, base_url: str) -> bool:
    r = simulate_self_test(session, base_url, zc_ok=True, triac_ok=True, source_ok=True)
    return r.status_code != 404


def restore_passing_self_test(session: requests.Session, base_url: str) -> None:
    """Clear safety lockout via HIL-only simulate route (no-op on production firmware)."""
    r = simulate_self_test(session, base_url, zc_ok=True, triac_ok=True, source_ok=True)
    if r.status_code == 404:
        return
    r.raise_for_status()


def require_simulate_api(session: requests.Session, base_url: str) -> None:
    r = simulate_self_test(session, base_url, zc_ok=True, triac_ok=True, source_ok=True)
    if r.status_code == 404:
        pytest.skip("HIL self-test simulate route not available (flash pio run -e hil)")
    r.raise_for_status()


def assert_safety_lockout_error(response: requests.Response) -> dict[str, Any]:
    assert response.status_code == 403, response.text[:500]
    body = response.json()
    assert body.get("error") == "safety_lockout", body
    assert body.get("missing_cap") == "safety_lockout", body
    assert body.get("message"), body
    return body


def device_surplus_regulation(session: requests.Session, base_url: str) -> bool:
    """True when the flashed profile compiles surplus routing (routers, not meter gateways)."""
    from profile_matrix import fetch_device

    dev = fetch_device(session, base_url)
    fw = (dev.get("capabilities") or {}).get("firmware_capabilities") or {}
    return fw.get("surplus_regulation") is True


def wait_self_test_idle(
    session: requests.Session,
    base_url: str,
    *,
    timeout_s: float = 45,
    poll_s: float = 0.5,
) -> None:
    """Wait until commissioning self-test is not pending or running."""
    deadline = time.time() + timeout_s
    url = f"{base_url.rstrip('/')}/api/v1/health"
    while time.time() < deadline:
        health = get_with_retry(session, url, timeout=20).json()
        st = health.get("self_test") or {}
        if not st.get("pending") and not st.get("running"):
            return
        time.sleep(poll_s)
    pytest.fail("commissioning self-test still running after timeout")


def regulation_required() -> bool:
    from hil_env import hil_env

    return hil_env("BALANSUN_HIL_REQUIRE_REGULATION", default="").strip() in (
        "1",
        "true",
        "yes",
    )


def skip_or_fail_not_regulating(pct_import: float, pct_export: float) -> None:
    if pct_import == 0.0 and pct_export == 0.0:
        msg = "Triac stayed at 0% — enable action 0 with auto schedule on HIL device"
        if regulation_required():
            pytest.fail(msg)
        pytest.skip(msg)


def load_json_fixture(*parts: str) -> Any:
    return json.loads((FIXTURES.joinpath(*parts)).read_text(encoding="utf-8"))
