#!/usr/bin/env python3
"""Merge scripts/fixtures/pmqtt-solar-router-full.json into device config and PUT (EEPROM persist)."""
from __future__ import annotations

import json
import os
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PRESET = Path(__file__).resolve().parent / "fixtures" / "pmqtt-solar-router-full.json"
DEFAULT_HOST = "http://192.168.2.159"


def _auth_headers() -> dict[str, str]:
    token = os.environ.get("BALANSUN_API_BEARER_TOKEN", "").strip()
    if token:
        return {"Authorization": f"Bearer {token}"}
    password = os.environ.get("BALANSUN_HIL_PASSWORD", "").strip()
    if not password:
        return {}
    body = json.dumps({"password": password}).encode()
    req = urllib.request.Request(
        f"{os.environ.get('BALANSUN_FIELD_URL', DEFAULT_HOST).rstrip('/')}/api/v1/auth/login",
        data=body,
        method="POST",
        headers={"Content-Type": "application/json"},
    )
    with urllib.request.urlopen(req, timeout=15) as r:
        token = json.load(r).get("token", "")
    return {"Authorization": f"Bearer {token}"} if token else {}


def _open(req: urllib.request.Request, timeout: int = 15):
    return urllib.request.urlopen(req, timeout=timeout)


def main() -> int:
    host = sys.argv[1] if len(sys.argv) > 1 else os.environ.get("BALANSUN_FIELD_URL", DEFAULT_HOST)
    host = host.rstrip("/")
    auth = _auth_headers()
    preset = json.loads(PRESET.read_text())
    req = urllib.request.Request(f"{host}/api/v1/config", headers=auth)
    with _open(req) as r:
        cfg = json.load(r)["config"]
    for k in ("mains_frequency_effective_hz", "mains_frequency_source", "mains_frequency_warning"):
        cfg.pop(k, None)
    for key in (
        "source",
        "mqtt_ip",
        "mqtt_port",
        "mqtt_user",
        "mqtt_password",
        "mqtt_repeat_sec",
        "pmqtt_topic",
        "pmqtt_schema",
        "pmqtt_bindings",
    ):
        if key in preset:
            cfg[key] = preset[key]
    body = json.dumps({"config": cfg}, separators=(",", ":")).encode()
    headers = {"Content-Type": "application/json", **auth}
    req = urllib.request.Request(f"{host}/api/v1/config", data=body, method="PUT", headers=headers)
    with _open(req, timeout=30) as r:
        print(r.read().decode())
    expected = len(preset.get("pmqtt_bindings", []))
    n = 0
    for attempt in range(5):
        time.sleep(1.0 if attempt == 0 else 2.0)
        try:
            req = urllib.request.Request(f"{host}/api/v1/config", headers=auth)
            with _open(req) as r:
                n = len(json.load(r)["config"].get("pmqtt_bindings", []))
        except (urllib.error.URLError, json.JSONDecodeError) as e:
            print(f"config GET attempt {attempt + 1} failed: {e}", file=sys.stderr)
            continue
        if n >= expected:
            break
    print(f"pmqtt_bindings on device: {n}")
    if expected and n < expected:
        print(f"expected at least {expected} bindings", file=sys.stderr)
        return 1
    req = urllib.request.Request(f"{host}/api/v1/measurements", headers=auth)
    with _open(req) as r:
        m = json.load(r)
    h = m["house"]
    print(
        "house day import/export:",
        h["energy_day_import_wh"],
        h["energy_day_export_wh"],
        "grid",
        h["grid_net_w"],
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
