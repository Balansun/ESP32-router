"""Load profile_test_matrix.json and assert device caps on HIL bench."""

from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Any

import requests

MATRIX_PATH = (
    Path(__file__).resolve().parents[1] / "golden" / "profile_test_matrix.json"
)

# Wires that appear in meter_pack_manifest; pick one absent from the flashed profile.
_CANDIDATE_UNSUPPORTED_SOURCES = (
    "Linky",
    "JsyMk194",
    "ShellyEm",
    "Pmqtt",
    "Analog",
    "Enphase",
)


def unsupported_source_for_row(row: dict[str, Any]) -> str | None:
    compiled = set(row.get("meters") or [])
    for wire in _CANDIDATE_UNSUPPORTED_SOURCES:
        if wire not in compiled:
            return wire
    return None


def load_matrix() -> dict[str, Any]:
    with MATRIX_PATH.open(encoding="utf-8") as f:
        return json.load(f)


def expected_profile_name() -> str | None:
    name = os.environ.get("BALANSUN_HIL_EXPECT_PROFILE", "").strip()
    return name or None


def row_for_profile(product_profile: str) -> dict[str, Any] | None:
    for v in load_matrix()["variants"]:
        if v["product_profile"] == product_profile:
            return v
    return None


def require_profile_row() -> dict[str, Any]:
    name = expected_profile_name()
    if not name:
        import pytest

        pytest.skip("BALANSUN_HIL_EXPECT_PROFILE not set")
    row = row_for_profile(name)
    if row is None:
        import pytest

        pytest.skip(f"unknown profile {name!r} in matrix")
    return row


def fetch_device(session: requests.Session, base_url: str) -> dict[str, Any]:
    from hil_helpers import get_with_retry

    r = get_with_retry(session, f"{base_url.rstrip('/')}/api/v1/device", timeout=20)
    r.raise_for_status()
    return r.json()


def assert_device_caps(device: dict[str, Any], row: dict[str, Any]) -> None:
    caps = device.get("capabilities") or {}
    assert caps.get("product_profile") == row["product_profile"]
    fw = caps.get("firmware_capabilities") or {}
    expected = row["caps"]
    assert fw.get("surplus_regulation") is expected["surplus_regulation"]
    assert fw.get("triac") is expected["triac"]
    assert fw.get("multi_action") is expected["multi_action"]
    assert fw.get("source_test_inject") is expected["source_test_inject"]
    meters = fw.get("meters") or []
    assert list(meters) == row["meters"]
