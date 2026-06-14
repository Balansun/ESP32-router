"""HIL: PATCH /api/v1/config then GET — verify settings persist in RAM."""

from __future__ import annotations

import pytest

from profile_matrix import require_profile_row
from hil_helpers import get_with_retry, request_with_retry

pytestmark = pytest.mark.hil


def _get_config(hil_session, hil_base_url: str) -> dict:
    r = get_with_retry(hil_session, f"{hil_base_url}/api/v1/config", timeout=20)
    assert r.status_code == 200, r.text
    return r.json()["config"]


def _pwm(cfg: dict, key: str, default: str | int) -> str | int:
    """Sparse config export omits PWM keys when they match factory defaults."""
    return cfg[key] if key in cfg else default


def test_router_name_patch_and_get_roundtrip(hil_session, hil_base_url):
    """Identity field roundtrip — all profiles (router and gateway)."""
    require_profile_row()

    cfg = _get_config(hil_session, hil_base_url)
    original = cfg.get("router_name", "")
    candidate = "HIL-Settings-Roundtrip"
    new_name = candidate if original != candidate else "HIL-Settings-Roundtrip-Alt"

    patch = request_with_retry(
        hil_session,
        "PATCH",
        f"{hil_base_url}/api/v1/config",
        json={"router_name": new_name},
        timeout=20,
    )
    assert patch.status_code == 200, patch.text

    updated = _get_config(hil_session, hil_base_url)
    assert updated.get("router_name") == new_name

    restore = request_with_retry(
        hil_session,
        "PATCH",
        f"{hil_base_url}/api/v1/config",
        json={"router_name": original},
        timeout=20,
    )
    assert restore.status_code == 200, restore.text
    assert _get_config(hil_session, hil_base_url).get("router_name") == original


def test_pwm_config_patch_and_get_roundtrip(hil_session, hil_base_url):
    """PWM select fields roundtrip — surplus-regulation routers only."""
    row = require_profile_row()
    if not row["caps"]["surplus_regulation"]:
        pytest.skip("gateway profile has no PWM settings card")

    cfg = _get_config(hil_session, hil_base_url)
    orig_mode = str(_pwm(cfg, "pwm_mode", "off"))
    orig_gpio = int(_pwm(cfg, "pwm_gpio", -1))

    new_mode = "independent" if orig_mode != "independent" else "follow_triac"
    new_gpio = 4 if orig_gpio != 4 else 5

    patch = request_with_retry(
        hil_session,
        "PATCH",
        f"{hil_base_url}/api/v1/config",
        json={"pwm_mode": new_mode, "pwm_gpio": new_gpio},
        timeout=20,
    )
    assert patch.status_code == 200, patch.text

    updated = _get_config(hil_session, hil_base_url)
    assert _pwm(updated, "pwm_mode", "off") == new_mode
    assert int(_pwm(updated, "pwm_gpio", -1)) == new_gpio

    restore = request_with_retry(
        hil_session,
        "PATCH",
        f"{hil_base_url}/api/v1/config",
        json={"pwm_mode": orig_mode, "pwm_gpio": orig_gpio},
        timeout=20,
    )
    assert restore.status_code == 200, restore.text
    restored = _get_config(hil_session, hil_base_url)
    assert str(_pwm(restored, "pwm_mode", "off")) == orig_mode
    assert int(_pwm(restored, "pwm_gpio", -1)) == orig_gpio
