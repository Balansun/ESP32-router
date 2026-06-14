#!/usr/bin/env python3
"""Apply scripts/fixtures/home-config.sanitized.json via PUT /api/v1/system/backup."""
from __future__ import annotations

import json
import os
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = Path(__file__).resolve().parent / "fixtures" / "home-config.sanitized.json"
DEFAULT_HOST = "http://192.168.2.159"
READONLY_CONFIG_KEYS = (
    "mains_frequency_effective_hz",
    "mains_frequency_source",
    "mains_frequency_warning",
)


def _deep_merge(base: dict[str, Any], overlay: dict[str, Any]) -> dict[str, Any]:
    out = dict(base)
    for key, value in overlay.items():
        if isinstance(value, dict) and isinstance(out.get(key), dict):
            out[key] = _deep_merge(out[key], value)
        else:
            out[key] = value
    return out


def _strip_readonly_config(doc: dict[str, Any]) -> None:
    cfg = doc.get("config")
    if isinstance(cfg, dict):
        for k in READONLY_CONFIG_KEYS:
            cfg.pop(k, None)


def _enrich_pmqtt_bindings(doc: dict[str, Any]) -> None:
    cfg = doc.get("config")
    if not isinstance(cfg, dict):
        return
    topic = str(cfg.get("pmqtt_topic") or "").strip()
    bindings = cfg.get("pmqtt_bindings")
    if not topic or not isinstance(bindings, list):
        return
    for binding in bindings:
        if not isinstance(binding, dict):
            continue
        binding.setdefault("topic", topic)
        binding.setdefault("format", "json")
        binding.setdefault("enabled", True)


def _auth_headers(host: str) -> dict[str, str]:
    token = os.environ.get("BALANSUN_API_BEARER_TOKEN", "").strip()
    if token:
        return {"Authorization": f"Bearer {token}"}
    password = os.environ.get("BALANSUN_HIL_PASSWORD", "").strip()
    if not password:
        return {}
    body = json.dumps({"password": password}).encode()
    req = urllib.request.Request(
        f"{host.rstrip('/')}/api/v1/auth/login",
        data=body,
        method="POST",
        headers={"Content-Type": "application/json"},
    )
    with urllib.request.urlopen(req, timeout=15) as r:
        token = json.load(r).get("token", "")
    return {"Authorization": f"Bearer {token}"} if token else {}


def _open(req: urllib.request.Request, timeout: int = 15):
    return urllib.request.urlopen(req, timeout=timeout)


def _load_payload() -> dict[str, Any]:
    doc = json.loads(FIXTURE.read_text())
    _strip_readonly_config(doc)
    _enrich_pmqtt_bindings(doc)
    wifi_password = os.environ.get("BALANSUN_LAB_WIFI_PASSWORD", "").strip()
    if wifi_password:
        wifi = doc.setdefault("wifi", {})
        if isinstance(wifi, dict):
            wifi["password"] = wifi_password
    overlay_path = os.environ.get("BALANSUN_HOME_FIXTURE_OVERLAY", "").strip()
    if overlay_path:
        overlay = json.loads(Path(overlay_path).read_text())
        _strip_readonly_config(overlay)
        _enrich_pmqtt_bindings(overlay)
        doc = _deep_merge(doc, overlay)
        _enrich_pmqtt_bindings(doc)
    return doc


def _verify(host: str, auth: dict[str, str]) -> bool:
    ok = True
    for attempt in range(5):
        time.sleep(1.0 if attempt == 0 else 2.0)
        try:
            req = urllib.request.Request(f"{host}/api/v1/config", headers=auth)
            with _open(req) as r:
                cfg = json.load(r).get("config", {})
            source = cfg.get("source")
            bindings = len(cfg.get("pmqtt_bindings", []))
            print(f"config: source={source!r} pmqtt_bindings={bindings}")
            if source != "Pmqtt":
                ok = False
        except (urllib.error.URLError, json.JSONDecodeError) as e:
            print(f"config GET attempt {attempt + 1} failed: {e}", file=sys.stderr)
            ok = False
            continue
        try:
            req = urllib.request.Request(f"{host}/api/v1/actions/config", headers=auth)
            with _open(req) as r:
                nb_actions = int(json.load(r).get("nb_actions") or 0)
            print(f"actions: nb_actions={nb_actions}")
            if nb_actions < 1:
                ok = False
        except (urllib.error.URLError, json.JSONDecodeError) as e:
            print(f"actions GET attempt {attempt + 1} failed: {e}", file=sys.stderr)
            ok = False
            continue
        if ok:
            return True
    return False


def main() -> int:
    host = (
        sys.argv[1]
        if len(sys.argv) > 1
        else os.environ.get("BALANSUN_HIL_URL", DEFAULT_HOST)
    ).rstrip("/")
    if not FIXTURE.is_file():
        print(f"fixture missing: {FIXTURE}", file=sys.stderr)
        return 1
    auth = _auth_headers(host)
    payload = _load_payload()
    body = json.dumps(payload, separators=(",", ":")).encode()
    headers = {"Content-Type": "application/json", **auth}
    req = urllib.request.Request(
        f"{host}/api/v1/system/backup",
        data=body,
        method="PUT",
        headers=headers,
    )
    try:
        with _open(req, timeout=120) as r:
            resp = json.load(r)
    except urllib.error.HTTPError as e:
        print(e.read().decode()[:800], file=sys.stderr)
        return 1
    if not resp.get("ok"):
        print(f"backup PUT failed: {resp}", file=sys.stderr)
        return 1
    print(f"backup applied to {host}")
    if not _verify(host, auth):
        print("verification failed: expected source Pmqtt and nb_actions >= 1", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
