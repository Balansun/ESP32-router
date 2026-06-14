"""HIL: PWM config validation on router vs gateway profiles."""

from __future__ import annotations

import pytest

from profile_matrix import require_profile_row
from hil_helpers import get_with_retry, request_with_retry

pytestmark = pytest.mark.hil


def test_invalid_pwm_mode_rejected(hil_session, hil_base_url):
    row = require_profile_row()
    if not row["caps"]["surplus_regulation"]:
        pytest.skip("gateway profile has no PWM regulation path")

    r = request_with_retry(
        hil_session,
        "PATCH",
        f"{hil_base_url}/api/v1/config",
        json={"pwm_mode": "not_a_real_mode"},
        timeout=20,
    )
    assert r.status_code in (400, 422), r.text


def test_invalid_pwm_gpio_rejected(hil_session, hil_base_url):
    row = require_profile_row()
    if not row["caps"]["surplus_regulation"]:
        pytest.skip("gateway profile")

    r = request_with_retry(
        hil_session,
        "PATCH",
        f"{hil_base_url}/api/v1/config",
        json={"pwm_gpio": 99},
        timeout=20,
    )
    assert r.status_code in (400, 422), r.text


def test_valid_pwm_select_values_accepted(hil_session, hil_base_url):
    row = require_profile_row()
    if not row["caps"]["surplus_regulation"]:
        pytest.skip("gateway profile")

    r = request_with_retry(
        hil_session,
        "PATCH",
        f"{hil_base_url}/api/v1/config",
        json={"pwm_mode": "independent", "pwm_gpio": 4},
        timeout=20,
    )
    assert r.status_code == 200, r.text

    cfg = get_with_retry(hil_session, f"{hil_base_url}/api/v1/config", timeout=20)
    assert cfg.status_code == 200, cfg.text
    body = cfg.json()["config"]
    assert body.get("pwm_mode") == "independent"
    assert body.get("pwm_gpio") == 4

    request_with_retry(
        hil_session,
        "PATCH",
        f"{hil_base_url}/api/v1/config",
        json={"pwm_mode": "off", "pwm_gpio": -1},
        timeout=20,
    )
