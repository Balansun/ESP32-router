"""HIL tests for MQTT/REST parity routes (task010)."""

from __future__ import annotations

import json
from pathlib import Path

import pytest
import requests

from hil_helpers import (
    assert_get_ok_or_telemetry_not_ready,
    get_measurements_or_skip_not_ready,
    get_with_retry,
    wait_hil_health,
)

GOLDEN_DIR = Path(__file__).resolve().parents[1] / "golden"


def test_get_telemetry_snapshot_contract(hil_session, hil_base_url):
    r = get_with_retry(hil_session, f"{hil_base_url}/api/v1/telemetry/snapshot", timeout=15)
    assert_get_ok_or_telemetry_not_ready(r, "/api/v1/telemetry/snapshot")
    if r.status_code != 200:
        return
    payload = r.json()
    spec = json.loads(
        (GOLDEN_DIR / "required_telemetry_snapshot_keys.json").read_text(encoding="utf-8")
    )
    for key in spec["required_keys"]:
        assert key in payload, f"missing {key} on /api/v1/telemetry/snapshot"


def test_measurements_diagnostics_parity(hil_session, hil_base_url):
    payload = get_measurements_or_skip_not_ready(hil_session, hil_base_url)
    diag = payload.get("diagnostics") or {}
    assert isinstance(diag, dict)
    for key in ("source_health", "source_stale"):
        assert key in diag, f"missing measurements.diagnostics.{key}"


def test_post_triac_override_auto(hil_session, hil_base_url):
    r = hil_session.post(
        f"{hil_base_url}/api/v1/triac/override",
        json={"command": "AUTO"},
        timeout=15,
    )
    assert r.status_code == 200, r.text
    body = r.json()
    assert body.get("ok") is True


def test_telemetry_snapshot_second_channel_when_measurements_live(
    hil_session, hil_base_url
):
    """Flat second_* on snapshot when nested measurements report CH2 (e.g. Pmqtt)."""
    measurements = get_measurements_or_skip_not_ready(hil_session, hil_base_url)
    second = measurements.get("second") or {}
    raw = measurements.get("raw_meter") or {}
    # Match firmware balansun_source_logic_second_channel_snapshot_visible (not merely key presence).
    has_ch2 = (
        float(raw.get("voltage_second_v") or 0) > 0.1
        or float(raw.get("current_second_a") or 0) > 0.01
        or int(second.get("active_import_w") or 0) != 0
        or int(second.get("active_export_w") or 0) != 0
    )
    if not has_ch2:
        pytest.skip("device has no live second-channel metering")

    snap_r = get_with_retry(
        hil_session, f"{hil_base_url}/api/v1/telemetry/snapshot", timeout=15
    )
    assert_get_ok_or_telemetry_not_ready(snap_r, "/api/v1/telemetry/snapshot")
    if snap_r.status_code != 200:
        pytest.skip("telemetry snapshot not ready")
    snapshot = snap_r.json()
    assert "second_active_import_w" in snapshot, snapshot
    assert "second_voltage_v" in snapshot, snapshot


def test_config_pin_map_fields(hil_session, hil_base_url):
    """Pin map lives on GET /api/v1/system/pins (sparse /api/v1/config omits pin_* on constrained tier)."""
    r = get_with_retry(hil_session, f"{hil_base_url}/api/v1/system/pins", timeout=15)
    assert r.status_code == 200, r.text
    pins = r.json()
    for key in (
        "pin_triac_dim",
        "pin_zero_cross",
        "pin_uart_rx",
        "pin_uart_tx",
        "pin_temp",
        "pin_analog0",
        "pin_analog1",
        "pin_analog2",
        "board_pin_profile",
    ):
        assert key in pins, f"missing pins.{key}"
    for key in (
        "pin_triac_dim",
        "pin_zero_cross",
        "pin_uart_rx",
        "pin_uart_tx",
        "pin_analog0",
        "pin_analog1",
        "pin_analog2",
    ):
        assert pins[key] >= 0, f"pins.{key} must be a concrete GPIO, got {pins[key]}"


def test_post_system_pins_reset(hil_session, hil_base_url):
    r = hil_session.post(f"{hil_base_url}/api/v1/system/pins/reset", timeout=15)
    assert r.status_code == 200, r.text
    body = r.json()
    assert body.get("ok") is True
    assert body.get("reboot_required") is True
    pins_r = get_with_retry(hil_session, f"{hil_base_url}/api/v1/system/pins", timeout=15)
    assert pins_r.status_code == 200, pins_r.text
    pins = pins_r.json()
    for key in (
        "pin_triac_dim",
        "pin_zero_cross",
        "pin_uart_rx",
        "pin_uart_tx",
        "pin_analog0",
        "pin_analog1",
        "pin_analog2",
    ):
        assert pins.get(key, -1) >= 0, f"after reset, pins.{key} must be concrete GPIO"


def test_get_system_pins(hil_session, hil_base_url):
    r = get_with_retry(hil_session, f"{hil_base_url}/api/v1/system/pins", timeout=15)
    assert r.status_code == 200, r.text
    pins = r.json()
    for key in (
        "pin_triac_dim",
        "pin_zero_cross",
        "pin_uart_rx",
        "pin_uart_tx",
        "pin_temp",
        "board_pin_profile",
        "pin_default_triac_dim",
    ):
        assert key in pins, f"missing pins.{key}"


def test_put_system_pins_requires_field(hil_session, hil_base_url):
    r = hil_session.put(f"{hil_base_url}/api/v1/system/pins", json={}, timeout=15)
    assert r.status_code == 400, r.text


def test_get_config_json_not_truncated(hil_session, hil_base_url):
    """Constrained-tier GET /config must remain valid JSON (not a truncated splice fragment)."""
    r = get_with_retry(hil_session, f"{hil_base_url}/api/v1/config", timeout=15)
    assert r.status_code == 200, r.text
    body = r.text.strip()
    assert body.startswith("{"), body[:120]
    assert len(body) > 80, f"config response suspiciously short: {len(body)} bytes"
    payload = r.json()
    assert "config" in payload
    assert isinstance(payload["config"], dict)


def test_put_system_pins_idempotent_triac(hil_session, hil_base_url):
    pins_r = get_with_retry(hil_session, f"{hil_base_url}/api/v1/system/pins", timeout=15)
    assert pins_r.status_code == 200, pins_r.text
    pins = pins_r.json()
    key = "pin_triac_dim"
    assert key in pins
    r = hil_session.put(
        f"{hil_base_url}/api/v1/system/pins",
        json={key: pins[key]},
        timeout=15,
    )
    assert r.status_code == 200, r.text
    assert r.json().get("ok") is True


def test_put_system_pins_rejects_pin_for_wrong_meter_source(hil_session, hil_base_url):
    dev_r = get_with_retry(hil_session, f"{hil_base_url}/api/v1/device", timeout=15)
    assert dev_r.status_code == 200, dev_r.text
    active = (dev_r.json().get("source") or "NotDef").strip()
    effective = active
    if active.lower() == "ext":
        effective = (dev_r.json().get("source_data") or active).strip()
    eff_key = effective.lower().replace("-", "")
    uart_sources = {"jsy_mk194", "jsy_mk333", "linky"}
    if eff_key in uart_sources:
        reject_key, reject_val = "pin_analog0", 34
    elif eff_key == "analog":
        reject_key, reject_val = "pin_uart_rx", 16
    else:
        pytest.skip(f"pin rejection case not defined for source {effective!r}")
    try:
        r = hil_session.put(
            f"{hil_base_url}/api/v1/system/pins",
            json={reject_key: reject_val},
            timeout=15,
        )
    except requests.ConnectionError:
        wait_hil_health(hil_session, hil_base_url)
        pytest.skip("device rebooting after prior pin map change")
    assert r.status_code == 400, r.text
    body = r.json()
    detail = (body.get("message") or body.get("error") or r.text).lower()
    assert reject_key in detail or "not applicable" in detail, detail


@pytest.mark.skip(reason="Schedules device reboot; run manually when validating reboot API")
def test_post_system_reboot(hil_session, hil_base_url):
    r = hil_session.post(f"{hil_base_url}/api/v1/system/reboot", timeout=15)
    assert r.status_code == 200, r.text
    body = r.json()
    assert body.get("ok") is True


def test_post_triac_override_invalid(hil_session, hil_base_url):
    r = hil_session.post(
        f"{hil_base_url}/api/v1/triac/override",
        json={"command": "not-a-valid-triac-cmd"},
        timeout=15,
    )
    assert r.status_code == 400, r.text
