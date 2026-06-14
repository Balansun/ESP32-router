from __future__ import annotations

import json
import os
from pathlib import Path

import pytest

GOLDEN_DIR = Path(__file__).resolve().parents[1] / "golden"


def test_unauthorized_returns_error_envelope(hil_base_url):
    import requests

    pub = requests.get(f"{hil_base_url.rstrip('/')}/api/v1/public", timeout=10).json()
    if not pub.get("http_auth", {}).get("enabled"):
        pytest.skip("HTTP API auth disabled on device")

    r = requests.get(
        f"{hil_base_url.rstrip('/')}/api/v1/device",
        headers={
            "Authorization": (
                "Bearer 0000000000000000000000000000000000000000000000000000000000000000"
            )
        },
        timeout=15,
    )
    if r.status_code == 401:
        pass
    elif os.environ.get("BALANSUN_HIL_PASSWORD"):
        r = requests.get(
            f"{hil_base_url.rstrip('/')}/api/v1/device",
            auth=("admin", "wrong-password"),
            timeout=15,
        )
    assert r.status_code == 401
    payload = r.json()
    spec = json.loads((GOLDEN_DIR / "required_error_keys.json").read_text(encoding="utf-8"))
    for key in spec["required_keys"]:
        assert key in payload
