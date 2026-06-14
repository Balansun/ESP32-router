from __future__ import annotations

import json

import pytest

from hil_helpers import FIXTURES, inject_payload, require_inject_api


@pytest.fixture
def inject_ready(hil_session, hil_base_url):
    require_inject_api(hil_session, hil_base_url)


def test_inject_valid_house(inject_ready, hil_session, hil_base_url):
    del inject_ready
    body = json.loads((FIXTURES / "inject" / "valid_house.json").read_text(encoding="utf-8"))
    r = inject_payload(hil_session, hil_base_url, body)
    assert r.status_code == 200, r.text


@pytest.mark.parametrize(
    "fixture_name",
    ["invalid_missing_house.json", "invalid_negative_power.json"],
)
def test_inject_validation_errors(inject_ready, hil_session, hil_base_url, fixture_name):
    del inject_ready
    body = json.loads((FIXTURES / "inject" / fixture_name).read_text(encoding="utf-8"))
    r = inject_payload(hil_session, hil_base_url, body)
    assert r.status_code == 400, r.text
    err = r.json()
    for key in ("error", "message"):
        assert key in err
