from __future__ import annotations

import os

import pytest

pytestmark = pytest.mark.skipif(
    os.environ.get("BALANSUN_HIL_EEPROM_ROUNDTRIP", "0") != "1",
    reason="Set BALANSUN_HIL_EEPROM_ROUNDTRIP=1 on dedicated lab hardware",
)


def test_config_marker_roundtrip(hil_session, hil_base_url):
  r = hil_session.get(f"{hil_base_url}/api/v1/config", timeout=15)
  assert r.status_code == 200
  cfg = r.json()["config"]
  marker = cfg.get("router_name", "")
  put = hil_session.put(
      f"{hil_base_url}/api/v1/config",
      json={"config": {"router_name": marker}},
      timeout=15,
  )
  assert put.status_code == 200, put.text
  save = hil_session.post(f"{hil_base_url}/api/v1/system/save-now", timeout=15)
  assert save.status_code == 200, save.text
